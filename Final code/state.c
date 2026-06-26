#include "state.h"
#include "fault.h"
#include "log.h"

const char *state_to_string(SystemState state)
{
    switch (state) {
        case STATE_NORMAL:   return "NORMAL";
        case STATE_DEGRADED: return "DEGRADED";
        case STATE_SAFE:     return "SAFE";
        default:             return "UNKNOWN";
    }
}

void evaluate_system_state(VehicleStatus *status, const FaultStatus *faults)
{
    SystemState new_state;
    int active     = active_fault_count(faults);
    int persistent = persistent_fault_count(faults);

    /* SAFE is a latch – once entered it stays until reset */
    if (status->system_state == STATE_SAFE) {
        /* Re-evaluate only if persistent faults are fully gone */
        if (persistent == 0 && active == 0) {
            new_state = STATE_NORMAL;
        } else {
            return; /* stay in SAFE */
        }
    } else if (persistent >= 2) {
        new_state = STATE_SAFE;
    } else if (active >= 1) {
        new_state = STATE_DEGRADED;
    } else {
        new_state = STATE_NORMAL;
    }

    if (new_state != status->system_state) {
        log_info("System state: %s → %s  (active faults=%d, persistent=%d)",
                 state_to_string(status->system_state),
                 state_to_string(new_state),
                 active, persistent);
        status->previous_state = status->system_state;
        status->system_state   = new_state;
    }
}

void reset_system_state(VehicleStatus *status)
{
    log_info("System reset requested. State cleared to NORMAL.");
    status->previous_state = status->system_state;
    status->system_state   = STATE_NORMAL;
}