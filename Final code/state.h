#ifndef STATE_H
#define STATE_H

#include "types.h"

/* ─── Function Prototypes ──────────────────────────────────── */

/**
 * evaluate_system_state() - Determine NORMAL / DEGRADED / SAFE.
 * Rules:
 *   0 active faults           → NORMAL
 *   1+ active faults          → DEGRADED
 *   2+ persistent faults      → SAFE  (latched until reset)
 *
 * Once SAFE, system stays SAFE until explicit reset.
 */
void evaluate_system_state(VehicleStatus *status, const FaultStatus *faults);

/**
 * reset_system_state() - Force state back to NORMAL (recovery / reset).
 */
void reset_system_state(VehicleStatus *status);

/**
 * state_to_string() - Human-readable state name.
 */
const char *state_to_string(SystemState state);

#endif /* STATE_H */