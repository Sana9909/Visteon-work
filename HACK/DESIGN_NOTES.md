# ECU Simulator - Design Notes

This document outlines the architecture, execution model, and safety logic of the Vehicle ECU Simulator, in compliance with the Hackathon C Problem Statement.

## 1. Scheduler Execution Model

The ECU follows a **cyclic executive model**, where a fixed sequence of tasks is performed in a precise, deterministic order during every cycle.

### Execution Order (Scheduler Flow)
1.  **Read & Validate Inputs**: Raw speed, temperature, gear, and mode are acquired and sanitized. Input layer logic ensures values stay within bounds (e.g., 0-200 km/h).
2.  **Update Vehicle Mode**: Evaluates the legality of mode transitions (e.g., ACC to IGNITION_ON). If illegal, it forces a `FAULT` mode.
3.  **Run Control Logic**: Core safety checks for overspeed, critical temperature, and gear validity.
4.  **Update Fault Manager**: Sets per-cycle bitwise flags and increments persistence counters for any detected issues.
5.  **Evaluate System State**: Determines the overall health (`NORMAL`, `DEGRADED`, or `SAFE`) based on active faults and cumulative counters.
6.  **Generate Logs**: Provides a structured summary of the cycle for diagnostics and performance profiling.

### Justification of Order
*   **Input First**: Processing must start with data validation. If control logic acts on "garbage" data (out-of-bounds inputs), the system could command dangerous actuators.
*   **Mode before Control**: Many control rules are mode-dependent. The system must know if it's in `IGNITION_ON` before evaluating engine-related faults.
*   **Evaluate State after Faults**: System health is a symptom of fault history. We must update fault counters *before* deciding if the system should escalate to `SAFE`.
*   **Log Last**: Logs must provide a snapshot of the *completed* cycle, including the results of all previous steps.

**What can go wrong if order changes?**
- If **State** is evaluated before **Fault Update**, the system might stay in `NORMAL` for one cycle even after a critical fault, delaying vital safety interventions.
- If **Control** runs before **Validation**, the overspeed logic might calculate based on a speed of "10,000" if sensors glitch, leading to erratic behavior.

## 2. Module Responsibilities

| Module | Responsibility |
| :--- | :--- |
| **Input** | Sanitizes raw data; maintains "last known valid" values for robustness. |
| **Mode** | Manages a finite state machine for operating modes (OFF, ACC, IGN, FAULT). |
| **Control** | Executes specific safety algorithms (e.g., temperature thresholding). |
| **Fault** | Uses bitwise flags for efficiency and counters for long-term tracking. |
| **State** | Logic to determine safety escalation (e.g., latching the `SAFE` state). |
| **System I/O** | MISRA-compliant abstraction layer to isolate logic from standard C libraries. |

## 3. State Transition & Fault Logic

### Fault Priority
When multiple faults occur, they are reported in the following order of criticality:
1. **Critical Overheat** (Highest)
2. **Invalid Gear / Mode**
3. **Overspeed**
4. **High Temperature** (Lowest)

### Safety Escalation (Safe-State)
- **NORMAL**: Zero active faults.
- **DEGRADED**: One active fault or minor threshold violations.
- **SAFE**: Two or more simultaneous critical faults, OR persistent faults across 3+ cycles.
- **Latching**: Once in `SAFE`, the system requires an explicit `System Reset` or a `MODE_OFF` transition with zero faults to return to `NORMAL`.

## 4. Persistent Cumulative Flow

The test runner utilizes a **Cumulative Execution Model**:
- **Vehicle Persistence**: Internal state (`FaultStatus`, `VehicleStatus`) is maintained across all test cases until a `"reset": true` flag is seen.
- **Input Delta**: JSON cycles only need to specify parameters that *change*. If a cycle only specifies `speed`, the previous `temperature` and `gear` values are retained.
