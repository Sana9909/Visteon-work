/**
 * @file control.c
 * @brief Control logic implementation — MISRA refactored.
 */

#include "control.h"
#include "fault.h"
#include "system_io.h"

void run_control_checks(const VehicleInput *input, VehicleStatus *status, FaultStatus *faults)
{
    if ((input != NULL) && (status != NULL) && (faults != NULL)) {
        (void)status;  /* unused - kept for interface consistency (MISRA Rule 2.7) */

        /* ---- Overspeed Check ---- */
        if (input->speed > OVERSPEED_THRESHOLD) {
            set_fault(faults, FAULT_OVERSPEED);
        } else {
            /* No fault */
        }

        /* ---- Temperature Check (critical takes precedence over high) ---- */
        if (input->temperature > CRITICAL_TEMP_THRESHOLD) {
            set_fault(faults, FAULT_OVERTEMP_CRITICAL);
        } else if (input->temperature > HIGH_TEMP_THRESHOLD) {
            set_fault(faults, FAULT_OVERTEMP_HIGH);
        } else {
            /* No temperature fault */
        }

        /* ---- Gear Validity Check ---- */
        if ((input->gear < GEAR_MIN) || (input->gear > GEAR_MAX)) {
            set_fault(faults, FAULT_INVALID_GEAR);
        } else {
            /* No gear fault */
        }
    }
}
