/**
 * @file state.c
 * @brief Safe-state manager implementation — MISRA refactored.
 */

#include "state.h"
#include "perf.h"
#include "system_io.h"

/**
 * @brief Count the number of set bits in a fault flags word.
 */
static uint8_t count_active_faults(FaultFlags flags)
{
    uint8_t count = 0U;
    uint32_t temp_flags = flags;
    
    while (temp_flags != 0U) {
        count += (uint8_t)(temp_flags & 1U);
        temp_flags >>= 1U;
    }
    return count;
}

void init_system(VehicleStatus *status, FaultStatus *faults)
{
    if ((status != NULL) && (faults != NULL)) {
        status->current_mode           = MODE_OFF;
        status->previous_mode          = MODE_OFF;
        status->system_state           = STATE_NORMAL;
        status->previous_state         = STATE_NORMAL;
        status->last_valid_speed       = 0;
        status->last_valid_temperature = 0;
        status->last_valid_gear        = 0;
        status->last_valid_mode        = MODE_OFF;

        faults->active_faults            = 0U;
        faults->overspeed_counter        = 0U;
        faults->overtemp_critical_counter = 0U;
        faults->overtemp_high_counter    = 0U;
        faults->invalid_gear_counter     = 0U;
        faults->illegal_mode_counter     = 0U;

        if (!g_suppress_io) {
            sys_log_info("ECU initialized - OFF mode, NORMAL state");
        }
    }
}

void evaluate_system_state(VehicleStatus *status, const FaultStatus *faults)
{
    if ((status != NULL) && (faults != NULL)) {
        SystemState new_state = STATE_UNKNOWN;
        const char *reason = "unknown";

        /* ---- SAFE State Latch (Fast-Path) ---- */
        if (status->system_state == STATE_SAFE) {
            if ((status->current_mode == MODE_OFF) && (faults->active_faults == 0U)) {
                new_state = STATE_NORMAL;
                reason = "recovery via MODE_OFF reset, no active faults";
            } else {
                /* Latched — skip all further analysis */
                status->current_mode = MODE_FAULT;
                return;
            }
        }
        /* ---- Normal Escalation Rules ---- */
        else if (faults->active_faults == 0U) {
            new_state = STATE_NORMAL;
            reason = "no active faults";
        }
        else {
            /* Lazy: only count bits when we know there are faults */
            uint8_t active = count_active_faults(faults->active_faults);
            if (active >= 2U) {
                new_state = STATE_SAFE;
                reason = "2+ simultaneous critical faults this cycle";
            }
            else if (faults->overtemp_critical_counter >= 2U) {
                new_state = STATE_SAFE;
                reason = "persistent critical overheat (counter >= 2)";
            }
            else if (faults->overspeed_counter >= 3U) {
                new_state = STATE_SAFE;
                reason = "persistent overspeed (counter >= 3)";
            }
            else if (faults->illegal_mode_counter >= 2U) {
                new_state = STATE_SAFE;
                reason = "persistent illegal mode transitions (counter >= 2)";
            }
            else {
                new_state = STATE_DEGRADED;
                reason = "one active fault or elevated counter";
            }
        }

        /* ---- Log state transition with reason ---- */
        if (new_state != status->system_state) {
            if (!g_suppress_io) {
                sys_printf("[STATE] %d -> %d | Reason: %s\n",
                       (int)status->system_state, (int)new_state, reason);
            }
        }

        status->previous_state = status->system_state;
        status->system_state = new_state;

        /* If we are in SAFE state, force the mode to FAULT for safety-critical containment */
        if (status->system_state == STATE_SAFE) {
            status->current_mode = MODE_FAULT;
        }
    }
}
