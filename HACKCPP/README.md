# 🚗 Adaptive Smart Cabin & Vehicle Health Monitoring System v1.0

> A production-grade, multi-threaded **C++17** CLI application that simulates, monitors, and validates a vehicle's smart cabin and structural health systems in **real-time** — built with RAII, polymorphic interfaces, Reader-Writer lock synchronization, and an asynchronous log pipe.

---

## 📋 Table of Contents

- [What Does This Project Do?](#-what-does-this-project-do)
- [Key Features](#-key-features)
- [Project Structure](#-project-structure)
- [Architecture](#-architecture)
- [C++17 Concepts Demonstrated](#-c17-concepts-demonstrated)
- [Sensors](#-sensors)
- [Alert System](#-alert-system)
- [Threading Model](#-threading-model)
- [Test Suite](#-test-suite)
- [Performance Results](#-performance-results)
- [Build & Run](#-build--run)
- [Configuration](#-configuration)
- [Logs & Output Files](#-logs--output-files)

---

## 🤔 What Does This Project Do?

Imagine the computer inside a car that watches all the sensors — engine temperature, battery, speed, tires, doors, and seatbelts — and immediately warns you if something goes wrong. This project simulates exactly that, but as a terminal application.

Every half second, six virtual sensors generate realistic readings. A background monitor checks those readings against safety thresholds and raises colour-coded alerts. A live dashboard refreshes on screen every 2 seconds. Everything gets logged to disk without slowing down the sensors. All of this runs simultaneously on four threads.

```
┌─────────────────────────────────────────────────────────────────────┐
│          ADAPTIVE SMART CABIN & VEHICLE HEALTH MONITOR              │
│                         SYSTEM v1.0                                 │
│    Modern C++17 — Threads · RAII · Smart Pointers · Templates       │
└─────────────────────────────────────────────────────────────────────┘

  🌡️  Engine Temp   :  87.3 °C   [████████░░]  NORMAL
  ⚡  Battery       :  12.6 V    [████████░░]  NORMAL
  🏎️  Speed         :  94.0 km/h [███████░░░]  NORMAL
  🎈  Tire Pressure :  31.8 PSI  [████████░░]  NORMAL
  🚪  Door Status   :  CLOSED             ✅  NORMAL
  🎗️  Seatbelt      :  LOCKED             ✅  NORMAL

  ⚠️  ACTIVE ALERTS: None   |   SYSTEM STATUS: 🟢 NORMAL
```

---

## ✨ Key Features

- **Real-time sensor simulation** — 6 sensors update every 500 ms with realistic RNG-driven values and 5% fault injection
- **Prioritised alert engine** — 6 severity levels (INFO → CRITICAL) with O(1) per-sensor lookup
- **Strategy pattern rules engine** — each sensor has its own isolated, extensible `AlertRule` subclass
- **Live ASCII dashboard** — ANSI colour-coded bar charts refresh every 2 seconds showing min/max/avg stats
- **Asynchronous log pipe** — disk writes are fully off the critical path via `SafeQueue<T>` producer-consumer queue
- **Reader-Writer lock sync** — `std::shared_mutex` lets multiple dashboard reads happen in parallel with sensor writes
- **10 automated test cases** — boundary, integration, and fault-injection tests with CSV reporting
- **Cycle-accurate profiling** — performance benchmarks showing up to **77.5% faster** than baseline C++ design
- **Graceful RAII shutdown** — all threads join and all files close automatically, even on exception

---

## 🌟 Advanced Automotive Bonus Features (C++17 Decoupled Architecture)

In addition to the core vehicle monitoring, this version fully integrates all **ten high-complexity advanced automotive systems** outlined in the project requirements:

1. **Driver Profile Management** (Medium):
   - Supports hot-swappable seat heights, mirror angles, target temperatures, and safety threshold configurations. Swaps profiles instantly at runtime through the interactive command menu option `[1]`.
   - **Code Path:** [DriverProfile.hpp](include/DriverProfile.hpp)
2. **JSON/XML Configuration Loading** (High):
   - Designed a custom, high-speed C++ JSON parser (`JsonParser.hpp`) to load external driver profiles and test suite configurations without importing bulky external libraries.
   - **Code Path:** [JsonParser.hpp](include/JsonParser.hpp)
3. **CAN Message Simulation** (High):
   - Packs real-time float values into **11-bit arbitration ID, 8-byte payload CAN frames** (with scale-and-offset serialization) and tracks CAN bus load (%), packet rates, and transmission error metrics.
   - **Code Path:** [CanBus.hpp](include/CanBus.hpp) \| [CanBus.cpp](src/CanBus.cpp)
4. **Adaptive Alert Prioritization** (Medium):
   - Rather than static alert sorting, a dynamic **0-100 `priorityScore` engine** evaluates alerts in real-time and contextually escalates alert priorities (e.g. elevating seatbelt and door hazards if the vehicle speed surpasses `10 km/h` or `100 km/h`).
   - **Code Path:** [Alert.hpp](include/Alert.hpp) \| [Alert.cpp](src/Alert.cpp)
5. **Diagnostic Trouble Code (DTC) Simulation** (High):
   - Tracks emission-critical trouble codes (`P0117`, `P0562`, `P0219`, `P0300`) and captures detailed **Freeze Frames** containing precise sensor snapshots at the exact breach tick. Fully implements standard OBD-II Services `$01`, `$03`, `$04`, `$07`, and `$09`.
   - **Code Path:** [DtcManager.hpp](include/DtcManager.hpp) \| [DtcManager.cpp](src/DtcManager.cpp)
6. **ECU Health Monitor** (Medium):
   - Simulates four virtual Electronic Control Units (`ECM`, `BCM`, `BMS`, `TPMS`) running in parallel with dynamic CPU/RAM footprints and diagnostics watchdogs.
   - **Code Path:** [EcuMonitor.hpp](include/EcuMonitor.hpp) \| [EcuMonitor.cpp](src/EcuMonitor.cpp)
7. **Performance Graph Statistics** (Medium):
   - Renders visual real-time ASCII historical speed sparkline graphs directly on the console dashboard.
   - **Code Path:** [Dashboard.hpp](include/Dashboard.hpp) \| [Dashboard.cpp](src/Dashboard.cpp)
8. **OTA Update Simulation** (High):
   - Performs over-the-air firmware flashing (from `v1.0` to `v2.0`) with robust **Safety Interlocks** that strictly block flashing if vehicle speed > 0 or if active DTC faults exist.
   - **Code Path:** [OtaSimulator.hpp](include/OtaSimulator.hpp) \| [OtaSimulator.cpp](src/OtaSimulator.cpp)
9. **Crash-Safe Event Recording** (High):
   - Circular EDR flight recorder that buffers 10 seconds of vehicle states and automatically flushes pre/post-crash telemetry securely to binary (`logs/crash_edr.bin`) and text logs on impact.
   - **Code Path:** [EdrRecorder.hpp](include/EdrRecorder.hpp) \| [EdrRecorder.cpp](src/EdrRecorder.cpp)
10. **Service-Oriented Communication Model** (High):
    - A SOME/IP and DDS-inspired publish-subscribe Service Bus that decouples telemetry gathering threads from alert dispatchers and logger pipes.
    - **Code Path:** [ServiceBus.hpp](include/ServiceBus.hpp) \| [ServiceBus.cpp](src/ServiceBus.cpp)

---

## 📁 Project Structure

```
C++_Hackathon/
│
├── src/                          # All implementation files
│   ├── main.cpp                  # Entry point: startup menu + system orchestration
│   ├── Sensor.cpp                # Abstract Sensor base + 6 concrete derived classes
│   ├── Alert.cpp                 # Alert class (Rule of Five) + AlertManager + Strategy rules
│   ├── Dashboard.cpp             # Live console renderer + VehicleStatistics tracker
│   ├── Logger.cpp                # EventLogger<T> template + async file I/O
│   ├── ThreadManager.cpp         # SafeQueue<T>, SharedVehicleData, ThreadManager
│   └── tests.cpp                 # 10 automated test cases (JSON-driven)
│
├── include/                      # All header files
│   ├── Sensor.hpp
│   ├── Alert.hpp
│   ├── Dashboard.hpp
│   ├── Logger.hpp
│   ├── ThreadManager.hpp
│   └── JsonParser.hpp            # Lightweight JSON parser (no external dependencies)
│
├── data/
│   ├── config.txt                # Runtime thresholds and update intervals
│   └── test_cases.json           # JSON definitions for all 10 test cases
│
├── logs/                         # Auto-generated at runtime
│   ├── vehicle_log.txt           # Timestamped event log
│   ├── test_suite_log.txt        # Detailed test execution log
│   ├── test_report.csv           # Pass/Fail per test case
│   └── performance_metrics.csv   # Cycle-time benchmarks per module
│
├── CMakeLists.txt                # CMake build configuration (C++17, cross-platform)
└── SmartVehicleMonitor.exe       # Pre-built Windows binary (run directly)
```

---

## 🏗️ Architecture

### How the system fits together

```
main()
  │
  ├── [Startup Menu] ──► Run all tests / Run one test / Skip to Dashboard
  │
  ├── Sensor Factory ──────────────────────► 6 × unique_ptr<Sensor>
  ├── AlertManager  (Strategy rules)
  ├── VehicleStatistics (min/max/avg)
  ├── Dashboard     (ASCII renderer)
  ├── EventLogger<string> (async file writer)
  │
  └── SharedVehicleData  ◄── sensor vector moved in (zero copy)
        │   shared_ptr keeps data alive across all threads
        │
        └── ThreadManager  ──► spawns 4 worker threads
              │
              ├── sensorThread    ──►  update() all sensors        [every 500 ms]
              ├── monitorThread   ──►  evaluate AlertRules          [every   1 s ]
              ├── dashboardThread ──►  render to stdout             [every   2 s ]
              └── loggerThread    ──►  flush SafeQueue → disk       [every   1 s ]
```

### Data flow & lock strategy

```
sensorThread  ──[unique_lock]──► write sensor values
monitorThread ──[unique_lock]──► write alert state
dashboardThread [shared_lock]──► read  (non-blocking, concurrent)
loggerThread  ──[SafeQueue ]──► independent — no dataMutex needed
```

`std::shared_mutex` allows multiple threads to read simultaneously while writes stay exclusive — this is the key reason dashboard rendering never blocks sensor updates.

---

## 🔬 C++17 Concepts Demonstrated

| C++ Feature | Where & How It Is Used |
|---|---|
| **Smart Pointers** | `unique_ptr<Sensor>` gives exclusive ownership of each sensor; `shared_ptr<SharedVehicleData>` keeps shared state alive across all threads |
| **Runtime Polymorphism** | Abstract `Sensor` base with 6 concrete derived classes; `AlertRule` interface with 6 strategy subclasses — new sensors/rules need zero changes to existing code |
| **RAII** | `ThreadManager` destructor joins all threads; `EventLogger` destructor closes the log file — cleanup is guaranteed even on exception |
| **Move Semantics** | Sensor `vector` is `std::move`d into `SharedVehicleData` at startup — zero copy, zero extra heap allocation |
| **Rule of Five** | `Alert` class implements copy constructor, move constructor, copy assignment, move assignment, and destructor |
| **Templates** | `EventLogger<T>` and `SafeQueue<T>` are fully generic — work with any message type |
| **`std::filesystem`** | Creates the `logs/` directory portably on Windows, Linux, and macOS |
| **`std::atomic<bool>`** | The `running` shutdown flag is read by all 4 threads lock-free |
| **`std::shared_mutex`** | Reader-Writer lock — dashboard threads take `shared_lock`; sensor/monitor threads take `unique_lock` |
| **`std::condition_variable`** | `SafeQueue::waitPop` blocks efficiently instead of busy-waiting |
| **Lambda expressions** | `AlertManager::filterAlerts(predicate)` accepts any callable for flexible alert queries |
| **`constexpr`** | All alert threshold values (110°C, 10V, 25 PSI, 120 km/h) are compile-time constants — zero runtime cost |
| **Operator Overloading** | `Alert`: `<` (priority sort), `==` (deduplication), `<<` (pretty-print); `Sensor`: `<<`; `VehicleStatistics`: `<<` |
| **Strategy Pattern** | `AlertRule` hierarchy decouples threshold logic from `AlertManager` — add a new rule by adding one class |
| **`[[nodiscard]]`** | Applied to every getter — the compiler warns if a caller ignores the return value |
| **Exception Safety** | Top-level `try/catch` in `main()` catches both `std::exception` and unknown exceptions |

---

## 🌡️ Sensors

Six physical vehicle quantities are simulated with realistic noise models and fault injection:

| Sensor | Class | Unit | Normal Range | Simulation Behaviour | Alert Trigger |
|---|---|:---:|:---:|---|---|
| 🌡️ Engine Temperature | `EngineTemperatureSensor` | °C | 50 – 110 | Gaussian distribution centred at 85°C (σ=10). 5% chance of overheating spike up to 130°C | > 110°C → **CRITICAL** |
| ⚡ Battery Voltage | `BatteryVoltageSensor` | V | 10 – 14.5 | Normal distribution centred at 12.8V (σ=0.8). 5% chance of parasitic drain dip to 8–10V | < 10.0V → **CRITICAL** |
| 🏎️ Vehicle Speed | `SpeedSensor` | km/h | 0 – 120 | Random-walk algorithm with ±5 km/h delta per tick — simulates realistic acceleration and braking | > 120 km/h → **WARNING** |
| 🎈 Tire Pressure | `TirePressureSensor` | PSI | 25 – 40 | Gaussian centred at 32 PSI (σ=2). 5% chance of flat-tire deflation to 20–25 PSI | < 25 PSI → **WARNING** |
| 🚪 Door Status | `DoorSensor` | — | CLOSED | Weighted Bernoulli trial: 80% CLOSED, 20% OPEN per tick | OPEN + Speed > 10 km/h → **CRITICAL** |
| 🎗️ Seatbelt Latch | `SeatbeltSensor` | — | LOCKED | Weighted Bernoulli trial: 70% LOCKED, 30% UNLOCKED per tick | UNLOCKED + moving → **MEDIUM** |

Each sensor uses a **lazily-seeded per-object `std::mt19937`** Mersenne Twister RNG from `std::random_device`, so every sensor produces independent, non-correlated readings.

---

## 🚨 Alert System

### Severity levels (lowest → highest)

```
INFO  ──►  LOW  ──►  MEDIUM  ──►  HIGH  ──►  WARNING  ──►  CRITICAL
```

### How AlertManager works internally

The `AlertManager` is designed for speed and flexibility:

- **Bounded active deque** — keeps the 50 most recent alerts; oldest are evicted automatically
- **Unbounded history vector** — full session record is never discarded
- **O(1) per-sensor lookup** — a flat `bool[6]` array indexed by `SensorType` lets the monitor thread check "is this sensor already in alert?" without searching the deque
- **Lambda-based filtering** — `filterAlerts(predicate)`, `filterBySeverity()`, `filterBySource()` for flexible queries anywhere in the codebase

### Overall system status

| Condition | Status |
|---|---|
| No active alerts | 🟢 `NORMAL` |
| Highest alert is WARNING or below | 🟡 `WARNING` |
| Any CRITICAL alert active | 🔴 `CRITICAL` |

---

## 🧵 Threading Model

Four worker threads run independently inside `ThreadManager`, coordinated through `SharedVehicleData`:

| Thread | What It Does | Runs Every |
|---|---|:---:|
| `sensorThread` | Calls `update()` on all 6 sensors to generate new readings | 500 ms |
| `monitorThread` | Runs each `AlertRule` strategy — compares readings to thresholds | 1000 ms |
| `dashboardThread` | Clears the console and renders the full UI with ANSI colours | 2000 ms |
| `loggerThread` | Drains `SafeQueue` and writes pending log entries to disk | 1000 ms |

**Key design decisions:**

- `std::shared_mutex` — dashboard thread uses `shared_lock` (read-only, never blocks other readers); sensor and monitor threads use `unique_lock` (exclusive write)
- `std::atomic<bool> running` — shutdown flag is read by all threads without any lock
- `SafeQueue<string>` — logger thread is completely decoupled from the data mutex; heavy disk I/O never delays sensor updates
- `ThreadManager` destructor calls `stop()` automatically — no thread is ever abandoned

---

## 🧪 Test Suite

10 automated test cases are defined in `data/test_cases.json` and run from the startup menu. All 10 pass:

| # | Test Case | Inputs | Expected | Result |
|:---:|---|---|---|:---:|
| 1 | Runtime Polymorphism & Sensor Interface | 6 sensors instantiated and polled | Correct types, count = 6 | ✅ Passed |
| 2 | Engine Coolant Overheat | Temp = 115°C | ENGINE OVERHEAT → CRITICAL | ✅ Passed |
| 3 | Battery Low Voltage | Battery = 9.99V | LOW BATTERY → CRITICAL | ✅ Passed |
| 4 | Tire Deflation Warning | Pressure = 24.99 PSI | LOW TIRE PRESSURE → WARNING | ✅ Passed |
| 5 | Overspeed Warning | Speed = 121 km/h | OVERSPEED WARNING | ✅ Passed |
| 6 | Door Open while Moving | Door = OPEN, Speed = 11 km/h | DOOR OPEN → CRITICAL | ✅ Passed |
| 7 | Seatbelt Unlocked while Moving | Seatbelt = UNLOCKED, Speed = 1 km/h | SEATBELT WARNING | ✅ Passed |
| 8 | All Non-Critical Alerts Together | Battery=9V, Speed=130, Pressure=20, Seatbelt=UNLOCKED | 4 simultaneous alerts → WARNING | ✅ Passed |
| 9 | All Alerts (Worst Case) | All thresholds breached simultaneously | 6 simultaneous alerts → CRITICAL | ✅ Passed |
| 10 | Boundary Value Verification | All values exactly at threshold boundaries | Only DOOR + SEATBELT fire → CRITICAL | ✅ Passed |

---

## ⚡ Performance Results

All benchmarks compiled with **`g++ -O3`**. The re-architected design achieves a **~70% overall reduction in CPU cycle counts** compared to a standard naive C++ implementation:

| Module | Standard C++ | First Pass | Re-Architected (`-O3`) | Improvement |
|:---|:---:|:---:|:---:|:---:|
| `instantiate_sensors` | 123,159 | 60,684 | **27,771** | **77.5% faster (4.4×)** |
| `update_polymorphic` | 35,598 | 27,735 | **12,100** | **66.0% faster (2.9×)** |
| `set_nominal_value` | 21,985 | 9,995 | **4,504** | **79.5% faster (4.9×)** |
| `evaluate_sensors` | 24,554 | 11,570 | **7,306** | **70.2% faster (3.4×)** |
| `trigger_critical_alert` | 27,570 | 13,757 | **8,606** | **68.8% faster (3.2×)** |
| `trigger_complex_alert` | 43,593 | 24,631 | **12,835** | **70.6% faster (3.4×)** |
| `construct_alert` | 365 | 383 | **156** | **57.3% faster (2.3×)** |

Key optimizations that drove these results: lazy RNG seeding, `constexpr` threshold constants, O(1) `bool[6]` alert index replacing deque search, and `std::shared_mutex` replacing a plain `std::mutex`.

---

## 🚀 Build & Run

### Prerequisites

| Tool | Minimum Version | Purpose |
|---|---|---|
| C++ Compiler | C++17 support | GCC ≥ 9 / Clang ≥ 10 / MSVC 2019+ |
| CMake | 3.14 | Build system (optional — see Method A) |

---

### ⚡ Method A — Direct g++ (Fastest, Recommended)

Best for MSYS2, MinGW, or any Linux/macOS terminal. One command builds and you are ready to run:

```bash
# Step 1 — Build
g++ -O3 -std=c++17 -Wall -Wextra -Wpedantic -DCONSOLIDATED_BUILD src/main.cpp src/tests.cpp src/Sensor.cpp src/Alert.cpp src/Logger.cpp src/Dashboard.cpp src/ThreadManager.cpp src/ServiceBus.cpp src/CanBus.cpp src/DtcManager.cpp src/EcuMonitor.cpp src/OtaSimulator.cpp src/EdrRecorder.cpp -Iinclude -o SmartVehicleMonitor.exe -pthread


# Step 2 — Run (Windows)
.\SmartVehicleMonitor.exe

# Step 2 — Run (Linux / macOS)
./SmartVehicleMonitor.exe
```

> ⚠️ **Always run from the project root folder** — the app uses relative paths for `data/` and `logs/`.

---

### 🛠️ Method B — CMake

Standard cross-platform build for Visual Studio, Ninja, or Make:

```bash
# Step 1 — Create build directory
mkdir build && cd build

# Step 2 — Configure
cmake ..

# Step 3 — Build
cmake --build . --config Release

# Step 4 — Run (Windows)
.\SmartVehicleMonitor.exe

# Step 4 — Run (Linux / macOS)
./SmartVehicleMonitor
```

**Windows with Visual Studio (Developer Command Prompt):**

```bat
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
.\build\Release\SmartVehicleMonitor.exe
```

---

### 🖥️ Startup Menu

On launch you will see:

```
  ================================================================
  |                     STARTUP SELECTION                        |
  ================================================================
    [1] Run all 10 automated test cases
    [2] Select and run a specific test case
    [3] Skip tests and start Dashboard directly
  ----------------------------------------------------------------
    Enter option [1-3]:
```

Choose **1** to validate the system first, or **3** to jump straight to the live dashboard.

Once the dashboard is running, press **ENTER** at any time to trigger a graceful shutdown:

```
  ================================================================
  |                    SHUTDOWN SUMMARY                          |
  ================================================================
  | Total Sensors Created  : 6
  | Total Alerts Generated : 14
  | Active Alerts          : 2
  | Log Entries Written    : 87
  ================================================================
  |        System shut down gracefully. Goodbye!                 |
  ================================================================
```

---

### 🧪 Independent User-Defined Free-Text Test Mode

You can run custom test cases independently and dynamically evaluate alerts and DTCs from the command line using free-text sentences:

```bash
# Syntax
.\SmartVehicleMonitor.exe -testmode -user "<descriptive input text>"

# Example A — Overheating & moving with door open
.\SmartVehicleMonitor.exe -testmode -user "Engine temperature is 115C, speed is 90kmh, door is open"

# Example B — Overspeed and seatbelt unlocked (no quotes needed)
.\SmartVehicleMonitor.exe -testmode -user speed 120kmh seatbelt unlocked

# Example C — Low battery and low tire pressure
.\SmartVehicleMonitor.exe -testmode -user battery 9.8V tire 22psi
```

**Key Features of the CLI Test Mode:**
- **Dynamic NLP Extraction**: Automatically extracts numeric thresholds (`115C`, `12.5V`, `90kmh`, `22psi`) and states (`door open`, `seatbelt unlocked`, `door closed`).
- **Nominal Fallbacks**: Any sensor not mentioned automatically defaults to its safe nominal values (e.g. 85°C temp, 12.6V battery, 50km/h speed, 32PSI tires, closed door, and locked seatbelt).
- **Execution Report**: Instantly resolves active alert conditions, severity scores, and OBD-II diagnostics, printing a beautiful color-coded evaluation report to stdout.
- **Audit Trails**: Automatically logs every user-defined run details into a persistent file: `logs/user_defined_test_log.txt`.

---

## ⚙️ Configuration

Edit `data/config.txt` to change thresholds and timings **without recompiling**:

```ini
# How often each thread wakes up (milliseconds)
sensor_update_interval    = 500
monitor_update_interval   = 1000
dashboard_refresh_interval= 2000
logger_flush_interval     = 1000

# Alert thresholds — change these to test different scenarios
engine_temp_critical      = 110.0   # °C
battery_voltage_low       = 10.0    # Volts
tire_pressure_low         = 25.0    # PSI
speed_high                = 120.0   # km/h
speed_door_warning        = 10.0    # km/h — speed above which open door = CRITICAL

# Logging
log_file_path             = logs/vehicle_log.txt
max_active_alerts         = 50
```

---

## 📄 Logs & Output Files

All log files are created automatically in the `logs/` directory at runtime:

| File | What It Contains |
|---|---|
| `logs/vehicle_log.txt` | Timestamped system events — startup, every alert fired, shutdown |
| `logs/test_suite_log.txt` | Step-by-step execution log from the test runner with pass/fail details |
| `logs/test_report.csv` | One row per test: Topic, Inputs, Expected Result, Actual Result |
| `logs/performance_metrics.csv` | Cycle min/max/average per module per test case — used for profiling |
