#include "fault.h"
#include "log.h"

void set_fault(FaultStatus *faults, FaultType type)
{
    if ((int)type >= FAULT_COUNT) return;
    if (!TEST_BIT(faults->fault_flags, (int)type)) {
        SET_BIT(faults->fault_flags, (int)type);
        faults->fault_counters[(int)type]++;
    }
}

void clear_fault(FaultStatus *faults, FaultType type)
{
    if ((int)type >= FAULT_COUNT) return;
    CLEAR_BIT(faults->fault_flags, (int)type);
}

void update_fault_status(FaultStatus *faults)
{
    int i;
    for (i = 0; i < FAULT_COUNT; i++) {
        if (TEST_BIT(faults->fault_flags, i)) {
            faults->persistent_cycles[i]++;
            if (faults->persistent_cycles[i] >= PERSISTENT_THRESHOLD) {
                if (!TEST_BIT(faults->persistent_flags, i)) {
                    SET_BIT(faults->persistent_flags, i);
                    log_fault("Fault [%s] is now PERSISTENT (%d cycles)",
                              fault_type_to_string((FaultType)i),
                              faults->persistent_cycles[i]);
                }
            }
        } else {
            faults->persistent_cycles[i] = 0;
            CLEAR_BIT(faults->persistent_flags, i);
        }
    }
}

const char *fault_type_to_string(FaultType type)
{
    switch (type) {
        case FAULT_OVERSPEED: return "OVERSPEED";
        case FAULT_OVERTEMP:  return "OVERTEMP";
        case FAULT_GEAR:      return "INVALID_GEAR";
        case FAULT_MODE:      return "INVALID_MODE";
        case FAULT_REDLINE:   return "REDLINE";
        case FAULT_STALL:     return "STALL";
        default:              return "UNKNOWN";
    }
}

int active_fault_count(const FaultStatus *faults)
{
    int i, count = 0;
    for (i = 0; i < FAULT_COUNT; i++) {
        if (TEST_BIT(faults->fault_flags, i)) count++;
    }
    return count;
}

int persistent_fault_count(const FaultStatus *faults)
{
    int i, count = 0;
    for (i = 0; i < FAULT_COUNT; i++) {
        if (TEST_BIT(faults->persistent_flags, i)) count++;
    }
    return count;
}