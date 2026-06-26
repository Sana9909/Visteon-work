/**
 * @file mode.c
 * @brief Mode controller implementation — MISRA refactored.
 */

#include "mode.h"
#include "fault.h"
#include "perf.h"
#include "system_io.h"
#include <stdbool.h>

/**
 * @brief Mode transition matrix.
 * Rows: Current Mode, Columns: Requested Mode
 * Value: true if transition allowed, false otherwise.
 */
static const bool TRANSITION_MATRIX[MODE_MAX + 1][MODE_MAX + 1] = {
    /* To: OFF,     ACC,     IGN_ON,  FAULT */
    {  true,    true,    false,   false }, /* From: OFF      */
    {  true,    true,    true,    false }, /* From: ACC      */
    {  true,    true,    true,    false }, /* From: IGN_ON   */
    {  false,   false,   false,   true  }  /* From: FAULT    */
};

void update_mode(VehicleStatus *status, const VehicleInput *input, FaultStatus *faults)
{
    if ((status != NULL) && (input != NULL) && (faults != NULL)) {
        status->previous_mode = status->current_mode;

        /* Constant-time lookup replaces switch-case pipeline stalls */
        if (TRANSITION_MATRIX[status->current_mode][input->requested_mode]) {
            status->current_mode = input->requested_mode;
        } else {
            set_fault(faults, FAULT_ILLEGAL_MODE);
            status->current_mode = MODE_FAULT;
        }
    }
}
