# Adaptive Smart Cabin & Vehicle Health Monitoring System — Design Document

> This document covers: **README**, **Build Instructions**, **Architecture Explanation**, **Thread Synchronization Explanation**, and **Design Decisions** — all derived from a complete codebase audit.

---

## Table of Contents

- [1. README Document](#1-readme-document)
- [2. Build Instructions](#2-build-instructions)
- [3. Architecture Explanation](#3-architecture-explanation)
- [4. Thread Synchronization Explanation](#4-thread-synchronization-explanation)
- [5. Design Document](#5-design-document)

---

# 1. README Document

## 🚗 Adaptive Smart Cabin & Vehicle Health Monitoring System v1.0

A multi-threaded **C++17** console application that simulates, monitors, and validates a vehicle's smart cabin and structural health systems in real-time — built with RAII, polymorphic sensor interfaces, mutex-based thread synchronization, and an asynchronous log pipe.

### What Does This Project Do?

This system simulates the embedded computer inside a vehicle that watches all its sensors — engine temperature, battery voltage, speed, tire pressure, doors, and seatbelts — and immediately warns the driver if something goes wrong.

Every **500 ms**, six virtual sensors generate realistic readings using stochastic models (Gaussian distributions, random walks, weighted Bernoulli trials, and fault injection). A background **monitor thread** checks those readings against safety thresholds and raises severity-coded alerts. A **live ASCII dashboard** refreshes on-screen every 2 seconds with ANSI colour coding. A dedicated **logger thread** writes everything to disk without slowing down the hot path. All of this runs simultaneously on **4 worker threads**.

```
  ================================================================
  |                                                              |
  |     ADAPTIVE SMART CABIN & VEHICLE HEALTH MONITORING        |
  |                      SYSTEM v1.0                            |
  |                                                              |
  |  Modern C++17 — Threads, RAII, Smart Pointers, Templates    |
  |                                                              |
  ================================================================
```

### Key Features

- **Real-time sensor simulation** — 6 sensors update every 500 ms with RNG-driven values and 5% fault injection probability
- **Prioritised alert engine** — 6 severity levels (`INFO` → `CRITICAL`) with threshold-based evaluation and cross-sensor logic
- **Live ASCII dashboard** — ANSI colour-coded tabular display refreshes every 2 s, showing current readings, active alerts, statistics, and alert history
- **Asynchronous log pipe** — disk writes are fully off the critical path via `SafeQueue<T>` producer-consumer queue
- **Mutex-based thread synchronization** — `std::mutex` with `std::lock_guard` protects shared state; `std::atomic<bool>` provides lock-free shutdown signalling
- **10 automated test cases** — data-driven JSON-loaded tests with CSV reporting and `__rdtsc` cycle-accurate profiling
- **Graceful RAII shutdown** — all threads join and all files close automatically, even on exception
- **Zero external dependencies** — includes a custom header-only JSON parser; no Boost, nlohmann, or other libraries required

### Project Structure

```
HACKCPP/
│
├── src/                          # Implementation files (.cpp)
│   ├── main.cpp                  # Entry point: startup menu + system orchestration
│   ├── Sensor.cpp                # Abstract Sensor base + 6 concrete derived classes
│   ├── Alert.cpp                 # Alert class (Rule of Five) + AlertManager evaluation
│   ├── Dashboard.cpp             # Live console renderer + VehicleStatistics tracker
│   ├── Logger.cpp                # Explicit template instantiation for EventLogger<string>
│   ├── ThreadManager.cpp         # SharedVehicleData ctor + 4 worker thread implementations
│   └── tests.cpp                 # 10 automated test cases (JSON-driven) + profiler
│
├── include/                      # Header files (.hpp)
│   ├── Sensor.hpp                # Abstract base + 6 derived sensor classes + enums
│   ├── Alert.hpp                 # Alert (Rule of Five) + AlertManager + severity enum
│   ├── Dashboard.hpp             # SensorStats, VehicleStatistics, Dashboard class
│   ├── Logger.hpp                # EventLogger<T> header-only template (RAII file I/O)
│   ├── ThreadManager.hpp         # SafeQueue<T>, SharedVehicleData, ThreadManager class
│   └── JsonParser.hpp            # Self-contained recursive-descent JSON parser
│
├── data/
│   ├── config.txt                # Runtime thresholds and update intervals
│   └── test_cases.json           # JSON definitions for all 10 test scenarios
│
├── logs/                         # Auto-generated at runtime
│   ├── vehicle_log.txt           # Timestamped system event log
│   ├── test_suite_log.txt        # Detailed test execution log
│   ├── test_report.csv           # Pass/Fail summary per test case
│   └── performance_metrics.csv   # Cycle-time benchmarks per module
│
├── CMakeLists.txt                # CMake build configuration (C++17, cross-platform)
├── code_size_metrics.txt         # Compiled binary segment sizes per component
└── SmartVehicleMonitor.exe       # Pre-built Windows binary
```

### Sensors

Six sensors are simulated with realistic noise models:

| Sensor | Class | Unit | Normal Range | Simulation Model | Alert Trigger |
|---|---|:---:|:---:|---|---|
| Engine Temperature | `EngineTemperatureSensor` | °C | 50–130 | Gaussian (μ=85, σ=10); 5% spike to 110–130°C | > 110°C → CRITICAL |
| Battery Voltage | `BatteryVoltageSensor` | V | 8–14.5 | Gaussian (μ=12.8, σ=0.8); 5% dip to 8–10V | < 10.0V → HIGH |
| Vehicle Speed | `SpeedSensor` | km/h | 0–160 | Random walk (±5 km/h per tick) | > 120 km/h → HIGH |
| Tire Pressure | `TirePressureSensor` | PSI | 20–40 | Gaussian (μ=32, σ=2); 5% deflation to 20–25 PSI | < 25 PSI → LOW |
| Door Status | `DoorSensor` | — | CLOSED | Bernoulli (80% closed, 20% open) | OPEN + speed > 10 → CRITICAL |
| Seatbelt Latch | `SeatbeltSensor` | — | LOCKED | Bernoulli (70% locked, 30% unlocked) | UNLOCKED + moving → MEDIUM |

### Alert System

**Severity levels** (lowest → highest):
```
INFO  →  LOW  →  MEDIUM  →  HIGH  →  WARNING  →  CRITICAL
```

**System status** is determined by the highest active alert:

| Condition | Status |
|---|---|
| No active alerts | `NORMAL` |
| Highest is HIGH or WARNING | `WARNING` |
| Any CRITICAL alert active | `CRITICAL` |

### Test Suite (10 Automated Test Cases)

| # | Description | Expected Result |
|:---:|---|---|
| 1 | Normal operation — all nominal values | No alerts, NORMAL |
| 2 | Engine overheat (115°C) | ENGINE OVERHEAT → CRITICAL |
| 3 | Low battery (9.99V) | LOW BATTERY → HIGH / WARNING |
| 4 | Overspeed (121 km/h) | OVERSPEED WARNING → HIGH |
| 5 | Door open while moving (11 km/h) | DOOR OPEN → CRITICAL |
| 6 | Seatbelt unlocked while moving | SEATBELT UNLOCKED → MEDIUM |
| 7 | Low tire pressure (24.99 PSI) | LOW TIRE PRESSURE → LOW |
| 8 | All non-critical alerts together | 4 simultaneous alerts → WARNING |
| 9 | All alerts together (worst case) | Multiple alerts → CRITICAL |
| 10 | Boundary value verification | Edge-case threshold validation |

### Configuration

Edit [config.txt](file:///c:/HACKCPP/data/config.txt) to change thresholds without recompiling:

```ini
sensor_update_interval=500       # ms between sensor updates
monitor_update_interval=1000     # ms between alert evaluations
dashboard_refresh_interval=2000  # ms between dashboard renders
logger_flush_interval=1000       # ms between log flushes

engine_temp_critical=110.0       # °C
battery_voltage_low=10.0         # Volts
tire_pressure_low=25.0           # PSI
speed_high=120.0                 # km/h
speed_door_warning=10.0          # km/h — door-open while above this speed = CRITICAL
```

### Logs & Output Files

| File | Contents |
|---|---|
| `logs/vehicle_log.txt` | Timestamped system events — startup, alerts, shutdown |
| `logs/test_suite_log.txt` | Step-by-step test execution with pass/fail details |
| `logs/test_report.csv` | One row per test: Topic, Inputs, Expected, Result |
| `logs/performance_metrics.csv` | Cycle min/max/avg per module per test case |

---

# 2. Build Instructions

## Prerequisites

| Tool | Minimum Version | Purpose |
|---|---|---|
| **C++ Compiler** | C++17 support | GCC ≥ 9, Clang ≥ 10, or MSVC 2019+ |
| **CMake** | 3.14+ | Build system generator (Method B only) |
| **Threads library** | POSIX pthreads or Win32 threads | Required for `std::thread`, `std::mutex` |

## Method A — Direct g++ (Fastest, Recommended)

Works with MSYS2/MinGW, Linux, or macOS. Single command to build:

```bash
# Build the main application + test suite (consolidated)
g++ -O2 -std=c++17 -Wall -Wextra -Wpedantic -DCONSOLIDATED_BUILD \
    src/main.cpp src/tests.cpp src/Sensor.cpp src/Alert.cpp \
    src/Logger.cpp src/Dashboard.cpp src/ThreadManager.cpp \
    -Iinclude -o SmartVehicleMonitor.exe -pthread

# Run (Windows / MSYS2)
./SmartVehicleMonitor.exe

# Run (Linux / macOS — no .exe extension needed)
./SmartVehicleMonitor
```

> [!IMPORTANT]
> The `-DCONSOLIDATED_BUILD` flag **must** be included when compiling `main.cpp` and `tests.cpp` together. This disables the standalone `main()` inside `tests.cpp` to avoid duplicate symbol errors.

**Building the test suite as a standalone executable:**

```bash
g++ -O2 -std=c++17 -Wall -Wextra -Wpedantic \
    src/tests.cpp src/Sensor.cpp src/Alert.cpp \
    src/Logger.cpp src/Dashboard.cpp src/ThreadManager.cpp \
    -Iinclude -o test_suite.exe -pthread

./test_suite.exe
```

## Method B — CMake (Cross-Platform)

```bash
# 1. Create build directory
mkdir build && cd build

# 2. Configure (auto-detects compiler and platform)
cmake ..

# 3. Build
cmake --build . --config Release

# 4. Run
./SmartVehicleMonitor          # Linux / macOS
.\SmartVehicleMonitor.exe      # Windows
```

**Windows with Visual Studio (Developer Command Prompt):**

```bat
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
.\build\Release\SmartVehicleMonitor.exe
```

**Windows with Ninja (fast):**

```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\SmartVehicleMonitor.exe
```

## CMake Configuration Details

The [CMakeLists.txt](file:///c:/HACKCPP/CMakeLists.txt) enforces the following:

| Setting | Value | Rationale |
|---|---|---|
| `CMAKE_CXX_STANDARD` | 17 | Required for `std::filesystem`, structured bindings, `if constexpr` |
| `CMAKE_CXX_STANDARD_REQUIRED` | ON | Build fails if compiler lacks C++17 |
| `CMAKE_CXX_EXTENSIONS` | OFF | Disables GNU extensions for strict portability |
| `Threads::Threads` | linked | Platform-appropriate threading library |
| `_CRT_SECURE_NO_WARNINGS` | WIN32 only | Suppresses MSVC warnings on `localtime` etc. |
| Warning flags | `/W4` (MSVC) or `-Wall -Wextra -Wpedantic` (GCC/Clang) | Maximum compiler diagnostics |

## Post-Build Verification

```bash
# Verify the build by running the test suite (option 1 from the startup menu)
./SmartVehicleMonitor.exe
# Select option [1] to run all 10 test cases
# Verify "10 / 10 Passed" output
```

> [!WARNING]
> **Always run from the project root directory.** The application uses relative paths (`data/test_cases.json`, `logs/`) and will fail with `JsonParser: Failed to open JSON file` if run from a different working directory.

---

# 3. Architecture Explanation

## High-Level System Overview

The system is organized as a **pipeline of four concurrent stages**, each running on its own thread, with a shared data hub acting as the central coordination point.

```
main()
  │
  ├──► [Startup Menu]
  │      ├── [1] Run all 10 automated tests
  │      ├── [2] Run a specific test
  │      └── [3] Skip to live dashboard
  │
  ├──► Sensor Factory ─────────────────────► 6 × unique_ptr<Sensor>
  ├──► AlertManager   (threshold evaluator)
  ├──► VehicleStatistics (min/max/avg tracker)
  ├──► Dashboard       (ANSI console renderer)
  ├──► EventLogger<string> (RAII file writer)
  │
  └──► SharedVehicleData  ◄── sensor vector moved in (zero-copy transfer)
          │   shared_ptr keeps data alive across all threads
          │
          └── ThreadManager  ──► spawns 4 worker threads
                │
                ├── sensorThread    ──►  update() all sensors         [every 500 ms]
                ├── monitorThread   ──►  evaluate alert conditions    [every   1 s ]
                ├── dashboardThread ──►  render to stdout             [every   2 s ]
                └── loggerThread    ──►  flush SafeQueue → disk       [every   1 s ]
```

## Component Breakdown

### 1. Sensor Layer — Polymorphic Hierarchy

**Files:** [Sensor.hpp](file:///c:/HACKCPP/include/Sensor.hpp) | [Sensor.cpp](file:///c:/HACKCPP/src/Sensor.cpp)

The sensor framework uses **runtime polymorphism** via an abstract base class with pure virtual functions:

```
              Sensor (abstract base)
             ┌──────────┐
             │ update()  │ ◄── pure virtual
             │ getValue()│ ◄── pure virtual
             │ display() │ ◄── pure virtual
             └──────────┘
                  ▲
     ┌────────────┼──────────────┐───────────────┐
     │            │              │               │
  EngineTemp  BatteryV     SpeedSensor    TirePressure
  Sensor      Sensor                      Sensor
                                 │               │
                            DoorSensor    SeatbeltSensor
```

**Key design decisions:**
- Each sensor owns a private `std::mt19937` RNG, seeded uniquely via `std::random_device` + atomic counter, ensuring independent non-correlated readings
- A **static counter** (`sensorCount_`) tracks live sensor instances across the process (incremented in ctor, decremented in dtor)
- `operator<<` is overloaded as a `friend` to delegate to the virtual `display()` method

### 2. Alert Layer — Rule of Five + Manager

**Files:** [Alert.hpp](file:///c:/HACKCPP/include/Alert.hpp) | [Alert.cpp](file:///c:/HACKCPP/src/Alert.cpp)

```
Alert (value type — Rule of Five)
  ├── severity_    : AlertSeverity enum
  ├── message_     : std::string
  ├── source_      : std::string (originating sensor name)
  ├── timestamp_   : chrono::system_clock::time_point
  └── alertCount_  : static int (live object tracker)

AlertManager (owns alert collections)
  ├── activeAlerts_    : std::deque<Alert>   (bounded, FIFO eviction)
  ├── alertHistory_    : std::vector<Alert>  (unbounded session record)
  ├── maxActiveAlerts_ : size_t (default 50)
  └── constexpr thresholds (ENGINE_TEMP=110°C, BATTERY_LOW=10V, etc.)
```

**Alert evaluation logic in `evaluateSensors()`:**
1. Locate the speed sensor via `std::find_if` (O(n), cached for the cycle)
2. For each sensor, compare value against its threshold
3. For cross-sensor checks (door-open-while-moving, seatbelt-while-moving), combine the sensor's state with the cached speed value
4. Deduplicate alerts: if the same source already has an active alert, skip
5. Auto-remove alerts when the triggering condition clears (recovery)

### 3. Logger — Header-Only Template with RAII

**Files:** [Logger.hpp](file:///c:/HACKCPP/include/Logger.hpp) | [Logger.cpp](file:///c:/HACKCPP/src/Logger.cpp)

`EventLogger<T>` is a **header-only class template** demonstrating:

- **RAII:** Constructor opens the log file, destructor flushes queued messages and closes it — guaranteed even on exceptions
- **Copy semantics deleted** (unique file handle ownership); **move semantics enabled**
- **Thread safety:** Every public method acquires `logMutex_` via `std::lock_guard`
- **Async producer/consumer:** `enqueue()` pushes messages; `processQueue()` drains them to disk
- **Lambda search:** `search(predicate)` uses `std::copy_if` for flexible log filtering

### 4. Dashboard — Stateless ANSI Renderer

**Files:** [Dashboard.hpp](file:///c:/HACKCPP/include/Dashboard.hpp) | [Dashboard.cpp](file:///c:/HACKCPP/src/Dashboard.cpp)

The dashboard is a **stateless renderer** (aside from a mutable `displayCount_` counter). It:
- Reads from Sensors, AlertManager, and VehicleStatistics
- Outputs formatted ANSI-coloured ASCII tables to any `std::ostream`
- Uses box-drawing characters and colour-coded severity badges
- Sections: Header → Sensor Readings → Active Alerts → Statistics Table → Alert History → Footer

`VehicleStatistics` uses a `std::map<string, SensorStats>` to track running min/max/sum/count for each named sensor, providing O(log n) lookup with automatic alphabetical ordering.

### 5. ThreadManager — Orchestration Hub

**Files:** [ThreadManager.hpp](file:///c:/HACKCPP/include/ThreadManager.hpp) | [ThreadManager.cpp](file:///c:/HACKCPP/src/ThreadManager.cpp)

```
SharedVehicleData
  ├── sensors        : vector<unique_ptr<Sensor>>   (moved from main)
  ├── alertManager   : AlertManager&                (reference)
  ├── statistics     : VehicleStatistics&           (reference)
  ├── logger         : EventLogger<string>&         (reference)
  ├── dashboard      : Dashboard&                   (reference)
  ├── dataMutex      : std::mutex                   (protects all above)
  ├── running        : std::atomic<bool>            (lock-free shutdown flag)
  └── logQueue       : SafeQueue<string>            (decoupled log buffer)

ThreadManager
  ├── threads_       : vector<std::thread>          (4 workers)
  ├── data_          : shared_ptr<SharedVehicleData>
  ├── start()        : launches 4 threads
  ├── stop()         : sets running=false, joins all
  └── ~ThreadManager(): calls stop() — RAII guarantee
```

### 6. JsonParser — Self-Contained Recursive-Descent Parser

**File:** [JsonParser.hpp](file:///c:/HACKCPP/include/JsonParser.hpp)

A header-only, zero-dependency JSON parser supporting: null, bool, number, string, array, and object types. Uses recursive-descent parsing with `JsonValue` as a variant-like node type. Used exclusively for loading `data/test_cases.json` test configurations.

### 7. Test Suite — Data-Driven Validation

**File:** [tests.cpp](file:///c:/HACKCPP/src/tests.cpp)

10 test cases defined in [test_cases.json](file:///c:/HACKCPP/data/test_cases.json), each specifying:
- Input sensor values to inject
- Expected active alerts (severity + message prefix)
- Expected system status

The test runner also includes a `PerformanceProfiler` class that uses `__rdtsc` (CPU timestamp counter) to measure cycle counts per operation, writing results to `logs/performance_metrics.csv`.

---

# 4. Thread Synchronization Explanation

## Overview

The system uses **4 concurrent worker threads** coordinated through a shared data hub (`SharedVehicleData`). Synchronization is achieved through three complementary mechanisms:

1. **`std::mutex` + `std::lock_guard`** — protects shared mutable state
2. **`std::atomic<bool>`** — lock-free shutdown signalling
3. **`SafeQueue<T>`** — independent thread-safe queue with condition variable

## Synchronization Primitives

### 1. `dataMutex` — Shared State Protection

```cpp
// In SharedVehicleData:
mutable std::mutex dataMutex;
```

This single mutex guards access to: **sensors**, **alertManager**, **statistics**, and **dashboard**. Every thread that reads or writes these must acquire `dataMutex` first.

**Lock acquisition pattern** (used consistently across all threads):

```cpp
{
    std::lock_guard<std::mutex> lock(data_->dataMutex);
    // ... critical section: read/write shared state ...
}
// Lock automatically released here (RAII)
// Sleep happens OUTSIDE the lock
std::this_thread::sleep_for(std::chrono::milliseconds(500));
```

> [!IMPORTANT]
> The lock is always released **before** the thread sleeps. This is a critical design decision — it minimizes the time the mutex is held, reducing contention between threads.

### 2. `std::atomic<bool> running` — Lock-Free Shutdown

```cpp
// In SharedVehicleData:
std::atomic<bool> running{true};
```

The `running` flag is polled by every worker thread at the top of its loop:

```cpp
while (data_->running.load()) {
    // ... do work ...
}
```

This uses **lock-free atomic reads** — no mutex contention on the hot path. When `stop()` is called:

```cpp
void ThreadManager::stop() {
    data_->running.store(false);   // Signal all threads to exit
    for (auto& t : threads_) {
        if (t.joinable()) t.join();  // Wait for clean termination
    }
    threads_.clear();
}
```

### 3. `SafeQueue<T>` — Thread-Safe Producer-Consumer Queue

```cpp
template<typename T>
class SafeQueue {
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};
```

The `SafeQueue` provides **three access patterns**:

| Method | Behaviour | Used By |
|---|---|---|
| `push(T value)` | Enqueue + notify one waiter | sensorThread, monitorThread |
| `tryPop(T& value)` | Non-blocking dequeue (returns false if empty) | loggerThread |
| `waitPop(T& value, timeout)` | Blocking dequeue with timeout; handles spurious wakeups | (available but not used in production) |

The `condition_variable` uses a **predicate wait** to handle spurious wakeups correctly:

```cpp
bool waitPop(T& value, std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!cv_.wait_for(lock, timeout, [this] { return !queue_.empty(); })) {
        return false;  // Timed out
    }
    value = std::move(queue_.front());
    queue_.pop();
    return true;
}
```

## Thread-by-Thread Synchronization Analysis

### sensorThread (500 ms interval)

```
┌─ acquire dataMutex (lock_guard) ─────────────────────────────────┐
│  for each sensor:                                                 │
│    sensor->update()          // generate new reading               │
│    statistics.update(name, value)  // update min/max/avg          │
│  if (iterationCount % 5 == 0):                                    │
│    logQueue.push(summary)    // SafeQueue has its OWN mutex       │
└──────────────────────────────────────────────────────────────────┘
sleep(500ms)   ◄── mutex is NOT held during sleep
```

### monitorThread (1000 ms interval)

```
┌─ acquire dataMutex (lock_guard) ─────────────────────────────────┐
│  alertManager.evaluateSensors(sensors)  // check all thresholds   │
│  alertManager.clearExpiredAlerts()       // prune stale alerts     │
│  if (new alerts detected):                                        │
│    logQueue.push(alert.toString())      // deferred logging       │
└──────────────────────────────────────────────────────────────────┘
sleep(1000ms)
```

### dashboardThread (2000 ms interval)

```
┌─ acquire dataMutex (lock_guard) ─────────────────────────────────┐
│  cout << ANSI_CLEAR_SCREEN                                        │
│  dashboard.display(sensors, alertManager, statistics, cout)       │
│  cout << "Press ENTER to stop..."                                 │
└──────────────────────────────────────────────────────────────────┘
sleep(2000ms)
```

### loggerThread (1000 ms interval)

```
logger.processQueue()          ◄── logger has its OWN internal mutex
┌─ logQueue.tryPop() loop ────────────────────────────────────────┐
│  while (logQueue.tryPop(message)):                               │
│    logger.enqueue(message)   ◄── logger's internal mutex         │
└──────────────────────────────────────────────────────────────────┘
logger.processQueue()          ◄── flush to disk
sleep(1000ms)
```

> [!TIP]
> The loggerThread **never acquires `dataMutex`**. It communicates exclusively through `SafeQueue` (which has its own mutex) and the `EventLogger` (which also has its own mutex). This design means **disk I/O never blocks sensor updates or dashboard rendering**.

## Data Flow Diagram

```
sensorThread ──┬──[dataMutex]──► sensors[]       ──[dataMutex]──► dashboardThread
               │                 statistics       ──[dataMutex]──►
               │
               └──[SafeQueue mutex]──► logQueue ──[SafeQueue mutex]──► loggerThread
                                                                           │
monitorThread ──[dataMutex]──► alertManager    ──[dataMutex]──►            │
               │                                                           │
               └──[SafeQueue mutex]──► logQueue ──────────────────────────►│
                                                                           │
                                                          logger.enqueue() │
                                                          [logger mutex]   ▼
                                                          logs/vehicle_log.txt
```

## Shutdown Sequence

```
1. User presses ENTER in main()
2. main() calls threadManager.stop()
3. stop() sets running.store(false)                    ◄── atomic, lock-free
4. Each worker thread's while(running.load()) exits    ◄── at next loop iteration
5. loggerThread performs final drain:
     - Pops all remaining messages from logQueue
     - Calls logger.processQueue() one final time
6. stop() calls t.join() for each thread               ◄── blocks until all finish
7. ThreadManager destructor clears threads_ vector
8. EventLogger destructor flushes remaining queue + writes footer + closes file
9. Sensor destructors decrement sensorCount_
10. All resources released in reverse construction order (RAII stack unwinding)
```

## Potential Improvements

> [!NOTE]
> The current design uses `std::mutex` (exclusive lock) for all shared state access. Since the `dashboardThread` only **reads** data, it could benefit from `std::shared_mutex` with `shared_lock` for read access, allowing the dashboard to render concurrently with other readers without blocking. This would reduce contention in high-frequency update scenarios.

---

# 5. Design Document

## 5.1 Problem Statement

Design a real-time vehicle monitoring system that continuously reads from multiple physical sensors, evaluates safety conditions, displays status to the driver, and logs all events — with all operations running concurrently without data corruption or race conditions.

## 5.2 Design Goals

1. **Extensibility** — Adding a new sensor type requires only creating a new derived class; no modifications to existing code (Open/Closed Principle)
2. **Thread Safety** — All concurrent access to shared state must be synchronized; no data races
3. **Decoupled I/O** — Disk writes must never block sensor updates or alert evaluation
4. **Resource Safety** — All resources (threads, files, memory) must be cleaned up automatically, even on exceptions
5. **Testability** — The system must support automated, data-driven testing with machine-readable output

## 5.3 Key Design Decisions

### Decision 1: Abstract Base Class vs. Variant/Templates for Sensors

**Chosen:** Abstract base class with virtual functions (`Sensor`)

**Rationale:** The sensor set is open-ended (new physical quantities may be added). Runtime polymorphism via `virtual` dispatch allows new sensors to be added by inheritance without touching consumer code. The vtable overhead (~8 bytes per object + one indirect call per method) is negligible for 6 sensors updating at 2 Hz.

### Decision 2: Single Mutex vs. Per-Component Locks

**Chosen:** Single `dataMutex` protecting all shared state

**Rationale:** With only 4 threads and lock hold times under 1 ms, a single mutex simplifies reasoning about correctness and avoids deadlock risks from lock ordering. The critical sections are short (update 6 sensors, evaluate 6 thresholds, render one screen) so contention is minimal.

### Decision 3: SafeQueue for Log Decoupling

**Chosen:** Separate `SafeQueue<string>` for log messages, consumed by a dedicated logger thread

**Rationale:** Disk I/O latency is unpredictable (1–100+ ms depending on OS and disk state). By pushing log messages into a lock-free-ish queue and draining them on a separate thread, we guarantee that sensor updates and alert evaluation never stall waiting for `fwrite()`.

### Decision 4: `std::atomic<bool>` for Shutdown

**Chosen:** Atomic boolean flag polled in every thread loop

**Rationale:** The shutdown flag is read far more often than written (once per loop iteration × 4 threads vs. one write at shutdown). An atomic load is essentially free on modern CPUs (no memory barrier needed for relaxed reads). Using a mutex-protected flag would add unnecessary contention.

### Decision 5: Value Semantics for Alert (Rule of Five)

**Chosen:** Alert is a value type with full copy/move support

**Rationale:** Alerts are stored in STL containers (`deque`, `vector`) which require copy or move. By implementing the Rule of Five explicitly, we ensure correct lifetime tracking (`alertCount_` static counter) and efficient moves (string guts are stolen, not deep-copied).

### Decision 6: Header-Only Template for EventLogger

**Chosen:** Full implementation in [Logger.hpp](file:///c:/HACKCPP/include/Logger.hpp); explicit instantiation in [Logger.cpp](file:///c:/HACKCPP/src/Logger.cpp)

**Rationale:** Templates must be visible at the point of instantiation. A header-only design allows `EventLogger<T>` to work with any streamable type. The explicit instantiation in `Logger.cpp` for `std::string` reduces compile times by ensuring only one copy of the commonly-used specialization exists.

### Decision 7: Custom JSON Parser vs. External Library

**Chosen:** Self-contained recursive-descent parser in [JsonParser.hpp](file:///c:/HACKCPP/include/JsonParser.hpp)

**Rationale:** Zero external dependencies. The parser supports the subset of JSON needed for test case definitions (objects, arrays, strings, numbers, booleans, null). For a hackathon context, avoiding package manager setup and version conflicts is a significant advantage.

## 5.4 C++17 Features Used

| Feature | Where Used | Purpose |
|---|---|---|
| `std::unique_ptr<Sensor>` | Sensor ownership in vector | Exclusive ownership, automatic cleanup |
| `std::shared_ptr<SharedVehicleData>` | Cross-thread data sharing | Shared ownership, prevents dangling references |
| `std::thread` + `std::mutex` | ThreadManager, EventLogger | Concurrent execution with synchronized access |
| `std::atomic<bool>` | `SharedVehicleData::running` | Lock-free shutdown signalling |
| `std::condition_variable` | `SafeQueue::waitPop` | Efficient blocking wait (no busy-loop) |
| `std::filesystem` | `main.cpp` — log directory creation | Portable directory creation across OSes |
| `std::chrono` | Alert timestamps, thread sleep intervals | High-resolution timing |
| Move semantics | Sensor vector into SharedVehicleData | Zero-copy data transfer |
| Rule of Five | Alert class | Correct copy/move in STL containers |
| Class templates | `EventLogger<T>`, `SafeQueue<T>` | Generic, reusable components |
| Lambda expressions | `filterAlerts()`, `evaluateSensors()`, `search()` | Inline predicates and callbacks |
| `constexpr` | Alert thresholds in AlertManager | Compile-time constant folding |
| Operator overloading | `<<`, `<`, `==` on Alert, Sensor, VehicleStatistics | Natural syntax for output and comparison |
| Structured bindings | `for (const auto& [name, s] : allStats)` | Clean map iteration |
| `#pragma once` | All headers | Portable include guard |

## 5.5 Binary Size Metrics

| Component | .text (code) | .data | .bss | Total |
|---|---:|---:|---:|---:|
| Sensor | 19,364 B | 0 B | 16 B | 19,380 B |
| Alert | 39,088 B | 16 B | 16 B | 39,120 B |
| Logger | 32,040 B | 16 B | 0 B | 32,056 B |
| Dashboard | 34,376 B | 16 B | 0 B | 34,392 B |
| ThreadManager | 42,244 B | 0 B | 0 B | 42,244 B |
| Main | 103,748 B | 16 B | 0 B | 103,764 B |
| Tests | 201,460 B | 64 B | 0 B | 201,524 B |
| **Total** | **472,320 B** | **128 B** | **32 B** | **472,480 B** |

Final linked executable: **~1.51 MB** (includes standard library, runtime, and debug symbols).
