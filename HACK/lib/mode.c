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
 * @brief Check if a mode transition is legal.
 * @return true if legal, false if illegal.
 */
static bool is_legal_transition(Mode current, Mode requested)
{
    bool legal = false;

    if (current == requested) {
        legal = true;   /* staying in same mode is always legal */
    } else {
        switch (current) {
            case MODE_OFF:
                legal = (requested == MODE_ACC);
                break;

            case MODE_ACC:
                legal = (requested == MODE_IGNITION_ON || requested == MODE_OFF);
                break;

            case MODE_IGNITION_ON:
                legal = (requested == MODE_ACC || requested == MODE_OFF);
                break;

            case MODE_FAULT:
                legal = false;   /* FAULT mode is locked - no transitions allowed */
                break;

            case MODE_UNKNOWN:
            default:
                legal = false;
                break;
        }
    }
    
    return legal;
}

void update_mode(VehicleStatus *status, const VehicleInput *input, FaultStatus *faults)
{
    if ((status != NULL) && (input != NULL) && (faults != NULL)) {
        status->previous_mode = status->current_mode;

        /* Fast-path: FAULT mode is locked — skip transition table entirely */
        if (status->current_mode == MODE_FAULT) {
            set_fault(faults, FAULT_ILLEGAL_MODE);
            return;
        }

        if (is_legal_transition(status->current_mode, input->requested_mode)) {
            status->current_mode = input->requested_mode;
        } else {
            if (!g_suppress_io) {
                sys_printf("[MODE] Illegal transition %d -> %d -> forcing FAULT mode\n",
                       (int)status->current_mode, (int)input->requested_mode);
            }
            set_fault(faults, FAULT_ILLEGAL_MODE);
            status->current_mode = MODE_FAULT;
        }
    }
}
