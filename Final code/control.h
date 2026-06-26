#ifndef CONTROL_H
#define CONTROL_H

#include "types.h"

/* ─── Function Prototypes ──────────────────────────────────── */

/**
 * run_control_checks() - Evaluate overspeed, temperature, gear faults.
 * Priority: Critical Overheat > Invalid Gear > Overspeed > High Temp.
 */
void run_control_checks(const VehicleInput *input,
                        VehicleStatus      *status,
                        FaultStatus        *faults);

#endif /* CONTROL_H */