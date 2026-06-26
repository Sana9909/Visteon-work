/**
 * @file control.h
 * @brief Control logic interface for overspeed, temperature, and gear checks.
 *
 * Evaluates vehicle inputs against safety thresholds and sets
 * corresponding fault flags when violations are detected.
 */

#ifndef CONTROL_H_
#define CONTROL_H_

#include "types.h"

/**
 * @brief Run all control checks for the current cycle.
 *
 * Evaluates:
 *   - Overspeed:           speed > 120 km/h
 *   - Critical overheat:   temperature > 110 degC
 *   - High temperature:    temperature > 95 degC (but <= 110)
 *   - Invalid gear:        gear outside 0-5 range
 *
 * Note: This function does NOT clear active_faults.
 * Faults are accumulated from all modules during a cycle.
 * Clearing happens at the start of each cycle in the scheduler.
 *
 * @param input   Pointer to validated VehicleInput.
 * @param status  Pointer to VehicleStatus (reserved for future mode-dependent checks).
 * @param faults  Pointer to FaultStatus (fault flags set on violations).
 */
void run_control_checks(const VehicleInput *input, VehicleStatus *status, FaultStatus *faults);

#endif /* CONTROL_H_ */
