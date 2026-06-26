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
        /* Branchless counter updates using bit-shifts to eliminate pipeline stalls */
        faults->overspeed_counter        += (uint16_t)((faults->active_faults >> 0U) & 1U);
        faults->overtemp_critical_counter += (uint16_t)((faults->active_faults >> 1U) & 1U);
        faults->overtemp_high_counter    += (uint16_t)((faults->active_faults >> 2U) & 1U);
        faults->invalid_gear_counter     += (uint16_t)((faults->active_faults >> 3U) & 1U);
        faults->illegal_mode_counter     += (uint16_t)((faults->active_faults >> 4U) & 1U);
    }
}
