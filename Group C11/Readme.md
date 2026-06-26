# Vehicle ECU Simulator

## 🚀 Overview
A high-performance, modular Embedded C application simulating a safety-critical Vehicle ECU. This simulator is optimized for **branchless execution** and provides a comprehensive **statistical profiling engine** for automotive control logic validation.

### Core Modules
- **Input Validation**: Hardened sensor range-checking with stability-aware bypass.
- **Mode Controller**: Hardware-like Transition Matrix (LUT) for constant-time state changes.
- **Control Logic**: Branchless bitwise fault evaluation for deterministic timing.
- **State Manager**: Escalation-based safety state machine (NORMAL → DEGRADED → SAFE).
- **Diagnostic Logger**: Character-perfect parity between terminal output and persistent logs.

---

## 🛠️ Build and Installation 

### Dependencies
- **MinGW-w64**: Required for `gcc` and `mingw32-make`.
- **System**: Optimized for Windows (PowerShell/CMD).

STEP 1 : 
git clone https://glcc-qa.git.visteon.com/visteon-hackathon-2026/c-hackathon/group-c11.git

STEP 2:
cd .\group-c11\

STEP 3:
git switch c11-codebase 

### Compilation
From the project root, execute:
```powershell
mingw32-make all
```
This builds the core library and the target executable: `ecu_sim.exe`.

---

## 🎮 Execution Modes

Run the simulator using `.\ecu_sim.exe`. You will be presented with a menu:

### **[0] Interactive Mode**
Manually input sensor values (`Speed`, `Temperature`, `Gear`) to observe real-time ECU reactions, clamping, and fault triage in a live environment.

### **[1] Full Auto-Run & Profile**
Executes the entire mandatory T1-T9 test suite. 
- **Statistical Significance**: Each test runs **20 times**.
- **Timing Isolation**: Console I/O is suppressed during cycles 1-19 to ensure accurate CPU cycle counts.
- **Logging**: The final iteration generates the diagnostic `log.txt`.

### **[2] Selective Test Execution**
Developers can choose a specific test case (e.g., T1 through T9) from the suite. This is ideal for iterative debugging of specific persistent fault scenarios.

---

## 📉 Performance & Optimization

The ECU simulator is designed for sub-35 cycle module efficiency through advanced techniques:

- **Branchless Logic**: `run_control_checks` and `update_fault_status` use bitwise arithmetic instead of `if` statements, eliminating CPU pipeline stalls.
- **Transition LUT**: Mode legalities are verified via a static bitmask matrix, ensuring constant-time validation regardless of complexity.
- **Statistical Profiling**: Performance metrics (Min/Max/Mean) are captured across 20 iterations to filter out OS-level context switching noise.

---

## 🧪 Extending the Test Suite

New test scenarios are added by modifying [test_cases.json](file:///c:/ECUDesignHackathon/test_cases.json).

### JSON Schema
| Field | Description |
| :--- | :--- |
| `test_id` | Unique string (e.g., "T10"). |
| `description` | Summary of the scenario. |
| `initial_mode` | Optional starting mode (default is `MODE_OFF`). |
| `cycles` | Array of cycle objects. Values are **Delta-based**; if a sensor isn't specified, it reuses the last valid value. |
| `expect` | Success criteria (Mode, State, and specific `active_faults`). |

### Example Cycle
```json
{
  "speed": 130, 
  "engine_temp": 115, 
  "label": "High speed overheat trigger" 
}
```

---

## 📊 Diagnostic Outputs

After a profiling run, the following reports are generated:

| File | Purpose |
| :--- | :--- |
| **`log.txt`** | Serial-like trace of all ECU steps and cycle summaries. |
| **`integration_test_report.csv`** | Pass/Fail summary of the simulation suite. |
| **`test_performance_ranking.csv`** | Comprehensive efficiency ranking of all test cases. |
| **`performance_[Name].csv`** | Module-level timing (Min/Max/Avg) for a specific test. |
| **`code_performance_report.csv`** | Global module performance across the entire session. |

---

## 📂 Project Structure
- `lib/`: Core ECU logic modules.
- `main.c`: Entry point and task scheduler.
- `Makefile`: Automates the library and application builds.
- `test_cases.json`: Source of truth for all automated test scenarios.
