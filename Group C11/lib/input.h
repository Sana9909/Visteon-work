/**
 * @file input.h
 * @brief Input handling and validation interface.
 *
 * Provides functions to read raw vehicle inputs and validate them
 * against defined ranges. Invalid values are logged and replaced
 * with the last known valid value (defensive programming).
 */

#ifndef INPUT_H_
#define INPUT_H_

#include "types.h"

/**
 * @brief Read raw vehicle inputs from the user (interactive mode).
 * @param input  Pointer to VehicleInput struct to populate.
 */
void read_inputs(VehicleInput *input);

/**
 * @brief Validate all input fields against defined ranges.
 *
 * Invalid values are:
 *   - Logged with a warning message
 *   - Replaced with the last known valid value from status
 *   - Faults raised where applicable (e.g., FAULT_INVALID_GEAR)
 *
 * @param input   Pointer to VehicleInput (may be modified if invalid).
 * @param status  Pointer to VehicleStatus for last-valid fallback values.
 * @param faults  Pointer to FaultStatus for fault flag setting.
 */
void validate_inputs(VehicleInput *input, VehicleStatus *status, FaultStatus *faults);

#endif /* INPUT_H_ */
