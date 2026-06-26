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
        (void)status;
        
        /* Branchless bitwise evaluation to minimize pipeline stalls */
        bool is_crit = (input->temperature > CRITICAL_TEMP_THRESHOLD);
        bool is_high = (input->temperature > HIGH_TEMP_THRESHOLD);
        
        faults->active_faults |= ((uint32_t)(input->speed > OVERSPEED_THRESHOLD)) ? FAULT_OVERSPEED : 0U;
        faults->active_faults |= is_crit ? FAULT_OVERTEMP_CRITICAL : 0U;
        faults->active_faults |= (is_high && !is_crit) ? FAULT_OVERTEMP_HIGH : 0U;
        faults->active_faults |= ((uint32_t)(input->gear < GEAR_MIN || input->gear > GEAR_MAX)) ? FAULT_INVALID_GEAR : 0U;
    }
}
