#include <stdio.h>
#include "mode.h"
#include "fault.h"
#include "log.h"

/* Legal transition table
 * legal_transitions[from][to] = 1 means allowed */
static const int legal_transitions[4][4] = {
    /* to:  OFF  ACC  IGN  FLT */
    /* OFF */  { 1,   1,   0,   1 },
    /* ACC */  { 1,   1,   1,   1 },
    /* IGN */  { 1,   1,   1,   1 },
    /* FLT */  { 1,   0,   0,   1 },
};

int is_transition_legal(VehicleMode from, VehicleMode to)
{
    if ((int)from > (int)MODE_FAULT || (int)to > (int)MODE_FAULT) return 0;
    return legal_transitions[(int)from][(int)to];
}

const char *mode_to_string(VehicleMode mode)
{
    switch (mode) {
        case MODE_OFF:          return "OFF";
        case MODE_ACC:          return "ACC";
        case MODE_IGNITION_ON:  return "IGNITION_ON";
        case MODE_FAULT:        return "FAULT";
        default:                return "UNKNOWN";
    }
}

void update_mode(VehicleStatus *status, const VehicleInput *input,
                 FaultStatus *faults)
{
    VehicleMode requested = input->requested_mode;
    VehicleMode current   = status->current_mode;

    if (requested == current) return; /* no change needed */

    if (is_transition_legal(current, requested)) {
        log_info("Mode transition: %s → %s",
                 mode_to_string(current), mode_to_string(requested));
        status->previous_mode = current;
        status->current_mode  = requested;
        clear_fault(faults, FAULT_MODE);
    } else {
        log_fault("Illegal mode transition %s → %s! Forcing FAULT mode.",
                  mode_to_string(current), mode_to_string(requested));
        set_fault(faults, FAULT_MODE);
        status->previous_mode = current;
        status->current_mode  = MODE_FAULT;
    }
}