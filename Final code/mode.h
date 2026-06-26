#ifndef MODE_H
#define MODE_H

#include "types.h"

/* ─── Function Prototypes ──────────────────────────────────── */

/**
 * update_mode() - Validate mode transition and update status.
 * Forces MODE_FAULT on illegal transition.
 * Sets FAULT_MODE in faults if illegal.
 */
void update_mode(VehicleStatus *status, const VehicleInput *input,
                 FaultStatus *faults);

/**
 * is_transition_legal() - Returns 1 if from→to transition is allowed.
 */
int is_transition_legal(VehicleMode from, VehicleMode to);

/**
 * mode_to_string() - Human-readable mode name.
 */
const char *mode_to_string(VehicleMode mode);

#endif /* MODE_H */