/**
 * @file fault.c
 * @brief Fault manager implementation — MISRA refactored.
 */

#include "fault.h"
#include "system_io.h"

void set_fault(FaultStatus *faults, uint32_t fault_bit)
{
    if (faults != NULL) {
        faults->active_faults |= fault_bit;
    }
}

void clear_fault(FaultStatus *faults, uint32_t fault_bit)
{
    if (faults != NULL) {
        faults->active_faults &= (~fault_bit);
    }
}

void clear_all_faults(FaultStatus *faults)
{
    if (faults != NULL) {
        faults->active_faults = FAULT_NONE;
    }
}

void increment_fault_counter(FaultStatus *faults, uint32_t fault_bit)
{
    if (faults != NULL) {
        if (fault_bit == FAULT_OVERSPEED) {
            faults->overspeed_counter++;
        }
        else if (fault_bit == FAULT_OVERTEMP_CRITICAL) {
            faults->overtemp_critical_counter++;
        }
        else if (fault_bit == FAULT_OVERTEMP_HIGH) {
            faults->overtemp_high_counter++;
        }
        else if (fault_bit == FAULT_INVALID_GEAR) {
            faults->invalid_gear_counter++;
        }
        else if (fault_bit == FAULT_ILLEGAL_MODE) {
            faults->illegal_mode_counter++;
        }
        else {
            /* Unknown fault bit - do nothing (MISRA) */
        }
    }
}

void update_fault_status(FaultStatus *faults)
{
    if (faults != NULL) {
        if ((faults->active_faults & FAULT_OVERSPEED) != 0U) {
            increment_fault_counter(faults, FAULT_OVERSPEED);
        }
        if ((faults->active_faults & FAULT_OVERTEMP_CRITICAL) != 0U) {
            increment_fault_counter(faults, FAULT_OVERTEMP_CRITICAL);
        }
        if ((faults->active_faults & FAULT_OVERTEMP_HIGH) != 0U) {
            increment_fault_counter(faults, FAULT_OVERTEMP_HIGH);
        }
        if ((faults->active_faults & FAULT_INVALID_GEAR) != 0U) {
            increment_fault_counter(faults, FAULT_INVALID_GEAR);
        }
        if ((faults->active_faults & FAULT_ILLEGAL_MODE) != 0U) {
            increment_fault_counter(faults, FAULT_ILLEGAL_MODE);
        }
    }
}
