#pragma once

// =============================================================================
// ThreadManager.hpp — Thread Management for Adaptive Smart Cabin System
// =============================================================================
// Demonstrates: std::thread, std::mutex, std::atomic, std::condition_variable,
//               RAII thread joining, template thread-safe queue, shared state
//
// Design Decisions:
//   - SafeQueue<T> is a template class providing a thread-safe producer/consumer
//     queue using mutex + condition_variable. It supports both blocking (waitPop)
//     and non-blocking (tryPop) consumption patterns.
//   - SharedVehicleData aggregates all shared state into a single struct so that
//     every thread function receives one coherent view of the system. The dataMutex
//     protects the sensors/alerts/statistics while the logQueue has its own
//     internal synchronisation (SafeQueue).
//   - ThreadManager is non-copyable and joins all threads in its destructor,
//     ensuring clean shutdown even if an exception propagates (RAII).
//   - The atomic<bool> running flag allows lock-free polling in tight loops
//     without contending on dataMutex.
// =============================================================================

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <memory>
#include <queue>
#include <functional>
#include <chrono>
#include <iostream>
#include <string>
#include "DriverProfile.hpp"

// Forward declarations — full definitions are only needed in the .cpp file
class Sensor;
class AlertManager;
class VehicleStatistics;
class Dashboard;
template<typename T> class EventLogger;

// =============================================================================
// SafeQueue<T> — Thread-Safe Queue
// =============================================================================
// A bounded-less FIFO queue guarded by a mutex. The condition variable allows
// consumer threads to sleep efficiently instead of busy-waiting.
//
// Template parameter T must be MoveConstructible.
// =============================================================================
template<typename T>
class SafeQueue {
private:
    std::queue<T> queue_;              // Underlying FIFO storage
    mutable std::mutex mutex_;         // Guards all access to queue_
    std::condition_variable cv_;       // Notified on every push()

public:
    // -------------------------------------------------------------------------
    // push — Enqueue a value (move semantics). Wakes one waiting consumer.
    // -------------------------------------------------------------------------
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    // -------------------------------------------------------------------------
    // tryPop — Non-blocking dequeue. Returns false if the queue is empty.
    // -------------------------------------------------------------------------
    bool tryPop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // -------------------------------------------------------------------------
    // waitPop — Blocking dequeue with timeout. Returns false on timeout.
    // Uses a predicate wait to handle spurious wakeups correctly.
    // -------------------------------------------------------------------------
    bool waitPop(T& value, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, timeout, [this] { return !queue_.empty(); })) {
            return false;  // Timed out — queue was still empty
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    // -------------------------------------------------------------------------
    // empty — Check emptiness (snapshot; may change immediately after return).
    // -------------------------------------------------------------------------
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    // -------------------------------------------------------------------------
    // size — Current queue depth (snapshot).
    // -------------------------------------------------------------------------
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

// =============================================================================
// SharedVehicleData — Centralised shared state for all worker threads
// =============================================================================
// Every thread function receives a shared_ptr<SharedVehicleData> so that
// lifetime is automatically managed. The struct holds:
//   - sensors       : owned vector of polymorphic Sensor objects
//   - alertManager  : reference to the single AlertManager instance
//   - statistics    : reference to the single VehicleStatistics instance
//   - logger        : reference to the EventLogger that writes to disk
//   - dashboard     : reference to the Dashboard renderer
//   - dataMutex     : protects sensors / alertManager / statistics / dashboard
//   - running       : atomic flag — set to false to request graceful shutdown
//   - logQueue      : thread-safe queue for log messages (decoupled from disk I/O)
// =============================================================================
struct SharedVehicleData {
    std::vector<std::unique_ptr<Sensor>> sensors;
    AlertManager& alertManager;
    VehicleStatistics& statistics;
    EventLogger<std::string>& logger;
    Dashboard& dashboard;

    mutable std::shared_mutex dataMutex;        // Protects sensors, alerts, stats, dashboard
    std::atomic<bool> running{true};     // Shutdown flag (lock-free)
    SafeQueue<std::string> logQueue;     // Decoupled log message buffer

    std::vector<DriverProfile> loadedProfiles;
    std::atomic<bool> isMenuMode{false};
    std::vector<double> speedHistory;

    // Constructor — moves the sensor vector in; references are bound at construction
    SharedVehicleData(std::vector<std::unique_ptr<Sensor>> s,
                      AlertManager& am,
                      VehicleStatistics& vs,
                      EventLogger<std::string>& el,
                      Dashboard& db);
};

// =============================================================================
// ThreadManager — Owns and orchestrates all worker threads
// =============================================================================
// Lifecycle:
//   1. Construct with a shared_ptr<SharedVehicleData>
//   2. Call start() to launch all 4 worker threads
//   3. Call stop()  to request shutdown and join all threads
//   4. Destructor calls stop() as a safety net (RAII guarantee)
//
// Thread roles:
//   - sensorThread     : updates sensor readings every 500 ms
//   - monitorThread    : evaluates alert conditions every 1 s
//   - dashboardThread  : refreshes the console display every 2 s
//   - loggerThread     : flushes the log queue to disk every 1 s
// =============================================================================
class ThreadManager {
private:
    std::vector<std::thread> threads_;             // Owned worker threads
    std::shared_ptr<SharedVehicleData> data_;      // Shared state

    // ----- Worker thread entry points (run on dedicated threads) -----
    void sensorThread();      // Updates sensors every 500 ms
    void monitorThread();     // Evaluates alerts every 1 s
    void dashboardThread();   // Refreshes display every 2 s
    void loggerThread();      // Writes log entries every 1 s

public:
    // Construct with shared vehicle data
    explicit ThreadManager(std::shared_ptr<SharedVehicleData> data);

    // RAII destructor — joins all threads that are still running
    ~ThreadManager();

    // Non-copyable, non-movable (threads cannot be copied)
    ThreadManager(const ThreadManager&) = delete;
    ThreadManager& operator=(const ThreadManager&) = delete;
    ThreadManager(ThreadManager&&) = delete;
    ThreadManager& operator=(ThreadManager&&) = delete;

    // Launch all worker threads
    void start();

    // Request shutdown and block until all threads have joined
    void stop();

    // Query the running flag (lock-free)
    bool isRunning() const;
};
