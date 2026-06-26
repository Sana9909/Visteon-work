/**
 * @file log.c
 * @brief Structured console logging implementation — MISRA refactored.
 */

#include "log.h"
#include "system_io.h"
#include <string.h>
#include <stdbool.h>

const char* mode_to_name(Mode m)
{
    const char *s;
    switch (m) {
        case MODE_OFF:         s = "OFF";         break;
        case MODE_ACC:         s = "ACC";         break;
        case MODE_IGNITION_ON: s = "IGNITION_ON"; break;
        case MODE_FAULT:       s = "FAULT";       break;
        case MODE_UNKNOWN:
        default:               s = "UNKNOWN";     break;
    }
    return s;
}

const char* state_to_name(SystemState s)
{
    const char *str;
    switch (s) {
        case STATE_NORMAL:   str = "NORMAL";   break;
        case STATE_DEGRADED: str = "DEGRADED"; break;
        case STATE_SAFE:     str = "SAFE";     break;
        case STATE_UNKNOWN:
        default:             str = "UNKNOWN";  break;
    }
    return str;
}

/**
 * @brief Write cycle summary to a given output stream.
 */
static void write_summary(sys_file_t out,
                           const VehicleInput *input,
                           const VehicleStatus *status,
                           const FaultStatus *faults)
{
    if ((input != NULL) && (status != NULL) && (faults != NULL)) {
        sys_fprintf(out, "\n=== CYCLE SUMMARY ===\n");

        sys_fprintf(out, "Input  : Speed=%d  Temp=%d  Gear=%d  Mode=%s\n",
                input->speed, input->temperature, input->gear,
                mode_to_name(input->requested_mode));

        sys_fprintf(out, "Status : Mode=%s (prev=%s)  State=%s\n",
                mode_to_name(status->current_mode),
                mode_to_name(status->previous_mode),
                state_to_name(status->system_state));

        sys_fprintf(out, "Faults : 0x%08X\n", faults->active_faults);

        sys_fprintf(out, "Counters: OS=%u  CritT=%u  HighT=%u  InvGear=%u  IllMode=%u\n",
                faults->overspeed_counter,
                faults->overtemp_critical_counter,
                faults->overtemp_high_counter,
                faults->invalid_gear_counter,
                faults->illegal_mode_counter);

        /* Priority-ordered fault reporting */
        if ((faults->active_faults & FAULT_OVERTEMP_CRITICAL) != 0U) {
            sys_fprintf(out, "  [PRIORITY 1] CRITICAL OVERHEAT\n");
        }
        if ((faults->active_faults & FAULT_INVALID_GEAR) != 0U) {
            sys_fprintf(out, "  [PRIORITY 2] INVALID GEAR\n");
        }
        if ((faults->active_faults & FAULT_ILLEGAL_MODE) != 0U) {
            sys_fprintf(out, "  [PRIORITY 2] ILLEGAL MODE TRANSITION\n");
        }
        if ((faults->active_faults & FAULT_OVERSPEED) != 0U) {
            sys_fprintf(out, "  [PRIORITY 3] OVERSPEED\n");
        }
        if ((faults->active_faults & FAULT_OVERTEMP_HIGH) != 0U) {
            sys_fprintf(out, "  [PRIORITY 4] HIGH TEMPERATURE\n");
        }

        if (faults->active_faults == 0U) {
            sys_fprintf(out, "  [OK] No active faults\n");
        }

        sys_fprintf(out, "======================\n");
    }
}

void log_cycle_summary(const VehicleInput *input,
                       const VehicleStatus *status,
                       const FaultStatus *faults,
                       sys_file_t logfile)
{
    write_summary(logfile, input, status, faults);
}

void log_cycle_timing(sys_file_t logfile, uint16_t cycle_num, const CycleTiming *timing)
{
    if ((logfile != NULL) && (timing != NULL)) {
        sys_fprintf(logfile, "\n  [PERF] Cycle %u  CPU Cycles (excluding printf):\n", (unsigned int)cycle_num);
        sys_fprintf(logfile, "    read_inputs           : %llu\n", (unsigned long long)timing->read_inputs);
        sys_fprintf(logfile, "    validate_inputs       : %llu\n", (unsigned long long)timing->validate_inputs);
        sys_fprintf(logfile, "    update_mode           : %llu\n", (unsigned long long)timing->update_mode);
        sys_fprintf(logfile, "    run_control_checks    : %llu\n", (unsigned long long)timing->run_control_checks);
        sys_fprintf(logfile, "    update_fault_status   : %llu\n", (unsigned long long)timing->update_fault_status);
        sys_fprintf(logfile, "    evaluate_system_state : %llu\n", (unsigned long long)timing->evaluate_system_state);
        sys_fprintf(logfile, "    TOTAL                 : %llu\n", (unsigned long long)timing->total);
    }
}

void fault_flags_to_csv_str(FaultFlags flags, char *buf, size_t size)
{
    if ((buf != NULL) && (size > 0U)) {
        sys_memset(buf, 0, size);
        if (flags == 0U) {
            sys_strncpy(buf, "NONE", size - 1U);
        } else {
            bool first = true;
            if ((flags & FAULT_OVERSPEED) != 0U) {
                if (!first) { strcat(buf, ", "); }
                strcat(buf, "OVERSPEED"); first = false;
            }
            if ((flags & FAULT_OVERTEMP_CRITICAL) != 0U) {
                if (!first) { strcat(buf, ", "); }
                strcat(buf, "CRIT_OVERHEAT"); first = false;
            }
            if ((flags & FAULT_OVERTEMP_HIGH) != 0U) {
                if (!first) { strcat(buf, ", "); }
                strcat(buf, "HIGH_TEMP"); first = false;
            }
            if ((flags & FAULT_INVALID_GEAR) != 0U) {
                if (!first) { strcat(buf, ", "); }
                strcat(buf, "INV_GEAR"); first = false;
            }
            if ((flags & FAULT_ILLEGAL_MODE) != 0U) {
                if (!first) { strcat(buf, ", "); }
                strcat(buf, "ILL_MODE"); first = false;
            }
        }
    }
}

Mode name_to_mode(const char *name)
{
    Mode m = MODE_OFF;
    if (name != NULL) {
        if ((strstr(name, "ACC") != NULL)) {
            m = MODE_ACC;
        } else if ((strstr(name, "IGNITION") != NULL)) {
            m = MODE_IGNITION_ON;
        } else if ((strstr(name, "FAULT") != NULL)) {
            m = MODE_FAULT;
        } else {
            m = MODE_OFF;
        }
    }
    return m;
}

SystemState name_to_state(const char *name)
{
    SystemState s = STATE_NORMAL;
    if (name != NULL) {
        if ((strstr(name, "DEGRADED") != NULL)) {
            s = STATE_DEGRADED;
        } else if ((strstr(name, "SAFE") != NULL)) {
            s = STATE_SAFE;
        } else {
            s = STATE_NORMAL;
        }
    }
    return s;
}

FaultFlags name_to_fault(const char *name)
{
    FaultFlags f = FAULT_NONE;
    if (name != NULL) {
        if ((strstr(name, "OVERSPEED") != NULL)) {
            f = FAULT_OVERSPEED;
        } else if ((strstr(name, "CRITICAL") != NULL) || (strstr(name, "CRIT_OVERHEAT") != NULL)) {
            f = FAULT_OVERTEMP_CRITICAL;
        } else if ((strstr(name, "HIGH_TEMP") != NULL) || (strstr(name, "HIGH") != NULL)) {
            f = FAULT_OVERTEMP_HIGH;
        } else if ((strstr(name, "INVALID_GEAR") != NULL) || (strstr(name, "INV_GEAR") != NULL)) {
            f = FAULT_INVALID_GEAR;
        } else if ((strstr(name, "ILLEGAL") != NULL) || (strstr(name, "ILL_MODE") != NULL)) {
            f = FAULT_ILLEGAL_MODE;
        }
    }
    return f;
}
