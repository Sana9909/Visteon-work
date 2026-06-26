/**
 * @file state.h
 * @brief Safe-state manager interface.
 *
 * Manages system state transitions: NORMAL -> DEGRADED -> SAFE.
 * Also provides system initialization.
 *
 * State Transition Rules:
 *   NORMAL:    No active faults this cycle.
 *   DEGRADED:  One active fault, or repeated warnings.
 *   SAFE:      Two or more simultaneous critical faults, OR
 *              persistent critical counters exceeded thresholds.
 *
 * SAFE State Latch:
 *   Once in SAFE, the system remains in SAFE until:
 *     - Mode is reset to MODE_OFF, AND
 *     - No active faults in the current cycle.
 *   This simulates a "power cycle" recovery in a real ECU.
 */

#ifndef STATE_H_
#define STATE_H_

#include "types.h"

/**
 * @brief Initialize the ECU system to default state.
 * Sets mode to OFF, state to NORMAL, clears all faults and counters.
 * @param status  Pointer to VehicleStatus to initialize.
 * @param faults  Pointer to FaultStatus to initialize.
 */
void init_system(VehicleStatus *status, FaultStatus *faults);

/**
 * @brief Evaluate and update the system state based on current faults.
 * Applies escalation rules and SAFE state latch logic.
 * Logs every state transition with the reason.
 * @param status  Pointer to VehicleStatus (system_state updated).
 * @param faults  Pointer to FaultStatus (read for evaluation).
 */
void evaluate_system_state(VehicleStatus *status, const FaultStatus *faults);

#endif /* STATE_H_ */
