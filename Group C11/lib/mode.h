/**
 * @file mode.h
 * @brief Mode controller interface.
 *
 * Manages vehicle operating mode transitions (OFF, ACC, IGNITION_ON, FAULT).
 * Enforces legal transition rules and forces FAULT mode on illegal attempts.
 */

#ifndef MODE_H_
#define MODE_H_

#include "types.h"

/**
 * @brief Evaluate and apply a mode transition.
 *
 * Checks if the requested mode transition is legal.
 * If legal, updates current_mode.
 * If illegal, forces MODE_FAULT and sets FAULT_ILLEGAL_MODE.
 * Always preserves previous_mode before updating.
 *
 * @param status  Pointer to VehicleStatus (mode fields updated).
 * @param input   Pointer to VehicleInput (requested_mode read).
 * @param faults  Pointer to FaultStatus (fault set on illegal transition).
 */
void update_mode(VehicleStatus *status, const VehicleInput *input, FaultStatus *faults);

#endif /* MODE_H_ */
