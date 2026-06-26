// =============================================================================
// ThreadManager.cpp — Thread Management Implementation
// =============================================================================
// Implements the four worker threads that drive the vehicle monitoring system:
//
//   1. sensorThread    — polls and updates every sensor at 500 ms intervals,
//                         feeds new readings into VehicleStatistics, and
//                         periodically enqueues a summary log line.
//
//   2. monitorThread   — evaluates alert rules against current sensor values
//                         every 1 s, detects *new* alerts by comparing counts,
//                         and enqueues each new alert as a log message.
//
//   3. dashboardThread — clears the console and redraws the full dashboard
//                         every 2 s while holding the data lock.
//
//   4. loggerThread    — drains two queues every 1 s:
//                           (a) the EventLogger's internal threaded queue
//                           (b) the SharedVehicleData::logQueue (SafeQueue)
//                         This keeps disk I/O off the hot paths.
//
// All lock scopes use std::lock_guard and are released *before* any sleep call
// to minimise contention. The atomic<bool> running flag is polled lock-free in
// every loop iteration.
// =============================================================================

#include "ThreadManager.hpp"
#include "Sensor.hpp"
#include "Alert.hpp"
#include "Dashboard.hpp"
#include "Logger.hpp"

#include "ServiceBus.hpp"
#include "CanBus.hpp"
#include "DtcManager.hpp"
#include "EcuMonitor.hpp"
#include "OtaSimulator.hpp"
#include "EdrRecorder.hpp"

#include <sstream>
#include <iomanip>
#include <algorithm>
#include <shared_mutex>
#include <cmath>

// =============================================================================
// SharedVehicleData constructor
// =============================================================================
// Moves the sensor vector into the struct and binds all manager references.
// The atomic running flag is initialised to true by its in-class initialiser.
SharedVehicleData::SharedVehicleData(
    std::vector<std::unique_ptr<Sensor>> s,
    AlertManager& am,
    VehicleStatistics& vs,
    EventLogger<std::string>& el,
    Dashboard& db)
    : sensors(std::move(s))
    , alertManager(am)
    , statistics(vs)
    , logger(el)
    , dashboard(db)
{
    // logQueue and dataMutex are default-constructed
    // running is initialised to true via the in-class default
}

// =============================================================================
// ThreadManager constructor
// =============================================================================
ThreadManager::ThreadManager(std::shared_ptr<SharedVehicleData> data)
    : data_(std::move(data))
{
    // Threads are not started here — call start() explicitly.
    // This separation allows the caller to finish any setup before threads run.
}

// =============================================================================
// ThreadManager destructor — RAII guarantee
// =============================================================================
// If the caller forgot to call stop(), the destructor handles it. This ensures
// no std::thread objects are destroyed while still joinable (which would call
// std::terminate).
ThreadManager::~ThreadManager()
{
    stop();
}

// =============================================================================
// start() — Launch all 4 worker threads
// =============================================================================
// Each thread captures `this` and calls the corresponding member function.
// We reserve space up front to avoid reallocations while threads might be
// reading threads_.size() (though we don't do that — belt and suspenders).
void ThreadManager::start()
{
    // Guard against double-start: only launch if no threads exist yet
    if (!threads_.empty()) {
        return;
    }

    // Ensure the running flag is set before any thread begins
    data_->running.store(true);

    threads_.reserve(4);

    threads_.emplace_back(&ThreadManager::sensorThread,    this);
    threads_.emplace_back(&ThreadManager::monitorThread,   this);
    threads_.emplace_back(&ThreadManager::dashboardThread, this);
    threads_.emplace_back(&ThreadManager::loggerThread,    this);
}

// =============================================================================
// stop() — Request shutdown and join all threads
// =============================================================================
// Setting the atomic flag to false causes every worker loop to exit on its next
// iteration. We then join each thread to ensure clean termination before the
// SharedVehicleData (and the objects it references) are destroyed.
void ThreadManager::stop()
{
    // Signal all threads to exit
    data_->running.store(false);

    // Join every joinable thread
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Clear the vector so a subsequent call to stop() is a no-op
    threads_.clear();
}

// =============================================================================
// isRunning() — Lock-free query of the shutdown flag
// =============================================================================
bool ThreadManager::isRunning() const
{
    return data_->running.load();
}

// =============================================================================
// sensorThread() — Update sensors and statistics every 500 ms
// =============================================================================
// Design notes:
//   - The lock is held only for the duration of the update loop, never during
//     the subsequent sleep.
//   - Every 5th iteration (~2.5 s) we enqueue a summary log message containing
//     the current reading of every sensor. This keeps the log file informative
//     without flooding it.
//   - We use Sensor::getName() and Sensor::getValueString() which are part of
//     the polymorphic interface, so derived-class specifics are handled
//     transparently.
static std::string getTimestampNowStr() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &time);
#else
    localtime_r(&time, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void ThreadManager::sensorThread()
{
    size_t iterationCount = 0;

    while (data_->running.load()) {
        // ---- Critical section: update sensors and statistics ----
        {
            std::unique_lock<std::shared_mutex> lock(data_->dataMutex);

            double tempVal = 0.0;
            double voltVal = 0.0;
            double speedVal = 0.0;
            double tireVal = 0.0;
            bool doorVal = false;
            bool seatbeltVal = false;

            for (auto& sensor : data_->sensors) {
                // Generate a new simulated reading
                sensor->update();

                // Feed the reading into the statistics accumulator
                data_->statistics.update(
                    sensor->getName(),
                    sensor->getValue()
                );

                // Extract values for EDR / CAN Bus encoding
                double val = sensor->getValue();
                SensorType type = sensor->getType();
                
                if (type == SensorType::ENGINE_TEMP) tempVal = val;
                else if (type == SensorType::BATTERY_VOLTAGE) voltVal = val;
                else if (type == SensorType::SPEED) speedVal = val;
                else if (type == SensorType::TIRE_PRESSURE) tireVal = val;
                else if (type == SensorType::DOOR) doorVal = (val == 1.0);
                else if (type == SensorType::SEATBELT) seatbeltVal = (val == 0.0);

                // Publish Event to Service Oriented Message Bus
                ServiceBus::getInstance().publish("sensors", SensorEvent{
                    sensor->getName(), val, sensor->getUnit(), getTimestampNowStr()
                });
            }

            // CAN Bus Signal Packing & Frame Simulation
            auto& can = CanBus::getInstance();
            CanFrame f1 = can.encodeSignal(0x101, tempVal, 0.1, -40.0); // Engine Temp
            CanFrame f2 = can.encodeSignal(0x102, voltVal, 0.01, 0.0);  // Battery Volt
            CanFrame f3 = can.encodeSignal(0x103, speedVal, 0.05, 0.0); // Speed
            CanFrame f4 = can.encodeSignal(0x104, tireVal, 0.1, 0.0);   // Tire Pressure
            
            can.transmitFrame(f1);
            can.transmitFrame(f2);
            can.transmitFrame(f3);
            can.transmitFrame(f4);

            // Record rolling Speed History for Performance Graph
            double lastSpeed = data_->speedHistory.empty() ? speedVal : data_->speedHistory.back();
            data_->speedHistory.push_back(speedVal);
            if (data_->speedHistory.size() > 20) {
                data_->speedHistory.erase(data_->speedHistory.begin());
            }

            // G-Force Calculation & Crash safe Recording
            double decelKmh = lastSpeed - speedVal;
            double gForce = 0.05 + 0.05 * std::abs(decelKmh);
            
            // Severe deceleration crash trigger (> 55 km/h instant drop)
            if (decelKmh > 55.0) {
                gForce = 4.2;
                EdrRecorder::getInstance().triggerCrashDump("Severe Collision Deceleration - Crash Event Detected");
            }

            EdrRecord edr{
                getTimestampNowStr(), tempVal, voltVal, speedVal, tireVal, doorVal, seatbeltVal, gForce
            };
            EdrRecorder::getInstance().recordTick(edr);

            // Every 5th iteration, build and enqueue a log summary
            ++iterationCount;
            if (iterationCount % 5 == 0) {
                std::ostringstream oss;
                oss << "[SENSOR UPDATE] Cycle " << iterationCount << " - ";
                for (const auto& sensor : data_->sensors) {
                    oss << sensor->getName() << ": "
                        << sensor->getValueString() << " "
                        << sensor->getUnit() << " | ";
                }
                data_->logQueue.push(oss.str());
            }
        }
        // ---- End critical section ----

        // Sleep outside the lock to avoid blocking other threads
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

// =============================================================================
// monitorThread() — Evaluate alert conditions every 1 s
// =============================================================================
// Design notes:
//   - We track the previous total alert count so that we can detect when new
//     alerts have been generated by evaluateSensors().
//   - For each new alert we enqueue a formatted log line into the SafeQueue.
//   - clearExpiredAlerts() is called each cycle to prune stale alerts, keeping
//     the active list manageable.
void ThreadManager::monitorThread()
{
    size_t previousAlertCount = 0;
    uint32_t localTicks = 0;

    while (data_->running.load()) {
        // ---- Critical section: evaluate alerts ----
        {
            std::unique_lock<std::shared_mutex> lock(data_->dataMutex);
            localTicks++;

            // Run OBD ECU health telemetry dynamic variance
            EcuMonitor::getInstance().updateTelemetry(localTicks);

            // Drive OTA software update ticks
            OtaSimulator::getInstance().tick();

            // Run the rule engine against current sensor values
            data_->alertManager.evaluateSensors(data_->sensors);

            // Dynamic Context Prioritization & Escalations
            double currentSpeed = 0.0;
            double tempVal = 0.0;
            double voltVal = 0.0;
            double tireVal = 0.0;
            bool doorVal = false;
            bool seatbeltVal = false;

            for (const auto& s : data_->sensors) {
                double val = s->getValue();
                SensorType type = s->getType();
                if (type == SensorType::SPEED) currentSpeed = val;
                else if (type == SensorType::ENGINE_TEMP) tempVal = val;
                else if (type == SensorType::BATTERY_VOLTAGE) voltVal = val;
                else if (type == SensorType::TIRE_PRESSURE) tireVal = val;
                else if (type == SensorType::DOOR) doorVal = (val == 1.0);
                else if (type == SensorType::SEATBELT) seatbeltVal = (val == 0.0);
            }
            data_->alertManager.prioritizeAlerts(currentSpeed);

            // Trigger OBD-II standard Diagnostic Trouble Codes (DTCs) based on alert signals
            for (const auto& alert : data_->alertManager.getActiveAlerts()) {
                if (alert.getSource() == "Engine Temperature") {
                    DtcManager::getInstance().triggerDtc("P0117", "Engine Coolant Temperature Circuit Low (Engine Overheating)", 
                                                       tempVal, voltVal, currentSpeed, tireVal, doorVal, seatbeltVal);
                } else if (alert.getSource() == "Battery Voltage") {
                    DtcManager::getInstance().triggerDtc("P0562", "System Voltage Low (Low Battery Charge)", 
                                                       tempVal, voltVal, currentSpeed, tireVal, doorVal, seatbeltVal);
                } else if (alert.getSource() == "Speed") {
                    DtcManager::getInstance().triggerDtc("P0219", "Engine Overspeed Condition (Safety Velocity Breach)", 
                                                       tempVal, voltVal, currentSpeed, tireVal, doorVal, seatbeltVal);
                } else if (alert.getSource() == "Door" && currentSpeed > 10.0) {
                    DtcManager::getInstance().triggerDtc("P0300", "Cabin Compartment Door Ajar (Door Open While Moving)", 
                                                       tempVal, voltVal, currentSpeed, tireVal, doorVal, seatbeltVal);
                }
            }

            // Clear any alerts that have expired (no longer relevant)
            data_->alertManager.clearExpiredAlerts();

            // Detect new alerts by comparing total count
            size_t currentAlertCount = data_->alertManager.getTotalAlertCount();
            if (currentAlertCount > previousAlertCount) {
                // Retrieve the full alert history and log the newest entries
                auto history = data_->alertManager.getAlertHistory();

                // The newest alerts are at the end of the history vector
                size_t newAlerts = currentAlertCount - previousAlertCount;
                size_t startIdx = (history.size() > newAlerts)
                                      ? history.size() - newAlerts
                                      : 0;

                for (size_t i = startIdx; i < history.size(); ++i) {
                    std::ostringstream oss;
                    oss << "[ALERT] " << history[i].toString();
                    data_->logQueue.push(oss.str());
                }

                previousAlertCount = currentAlertCount;
            }
        }
        // ---- End critical section ----

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

// =============================================================================
// dashboardThread() — Refresh the console display every 2 s
// =============================================================================
// Design notes:
//   - The dashboard renders to std::cout while holding the data lock, which
//     ensures a consistent snapshot of sensors, alerts, and statistics.
//   - We clear the screen using ANSI escape codes ("\033[2J\033[H") for a
//     flicker-minimised refresh. On Windows terminals that don't support ANSI,
//     the output will still be readable (just with extra escape characters).
//   - The 2 s interval is a compromise between responsiveness and readability.
void ThreadManager::dashboardThread()
{
    int renderTick = 20; // Start with rendering immediately

    while (data_->running.load()) {
        if (!data_->isMenuMode.load()) {
            if (renderTick >= 20) { // 20 * 100ms = 2000ms
                renderTick = 0;
                // ---- Critical section: render dashboard ----
                {
                    std::shared_lock<std::shared_mutex> lock(data_->dataMutex);

                    // Render the full dashboard system via the Dashboard class
                    data_->dashboard.renderSystem(
                        data_->sensors,
                        data_->alertManager,
                        data_->statistics,
                        data_->speedHistory
                    );
                }
                // ---- End critical section ----
            } else {
                renderTick++;
            }
        } else {
            renderTick = 20; // Instantly render next tick after menu mode finishes
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// =============================================================================
// loggerThread() — Drain log queues and flush to disk every 1 s
// =============================================================================
// Design notes:
//   - We drain TWO queues:
//       1. The EventLogger's own internal queue (via processQueue()). This
//          handles any messages that were enqueued directly through the logger.
//       2. The SharedVehicleData::logQueue (SafeQueue<string>). Worker threads
//          push messages here to avoid touching the logger under the data lock.
//   - The logger is NOT protected by dataMutex — it has its own internal
//     synchronisation. This is intentional: disk I/O should never block sensor
//     updates or dashboard rendering.
//   - We drain all pending messages from logQueue in a tight loop (no sleep
//     between individual pops) and then hand them to the logger.
void ThreadManager::loggerThread()
{
    while (data_->running.load()) {
        // Step 1: Process any messages already in the EventLogger's own queue
        data_->logger.processQueue();

        // Step 2: Drain the SharedVehicleData logQueue into the EventLogger
        std::string message;
        while (data_->logQueue.tryPop(message)) {
            // Use the logger's enqueue mechanism so it writes to the file
            data_->logger.enqueue(message);
        }

        // Process again to flush what we just enqueued
        data_->logger.processQueue();

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // ---- Final drain on shutdown ----
    // After running becomes false, there may still be messages in the queues.
    // Drain everything one last time to ensure no log messages are lost.
    std::string remaining;
    while (data_->logQueue.tryPop(remaining)) {
        data_->logger.enqueue(remaining);
    }
    data_->logger.processQueue();
}
