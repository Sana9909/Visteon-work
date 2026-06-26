#ifndef INPUT_H
#define INPUT_H

#include "types.h"

/* ─── Function Prototypes ──────────────────────────────────── */

/**
 * read_inputs() - Prompt user and read speed, temperature, gear, mode.
 * Stores raw values into *input without validation.
 */
void read_inputs(VehicleInput *input);

/**
 * validate_inputs() - Clamp / reject out-of-range values.
 * Preserves last known valid value on bad input.
 * Sets input->input_valid = 0 if any field was invalid.
 */
void validate_inputs(VehicleInput *input, VehicleStatus *status);

/**
 * init_last_valid() - Seed the last-valid shadow with safe defaults.
 */
void init_last_valid(void);

#endif /* INPUT_H */