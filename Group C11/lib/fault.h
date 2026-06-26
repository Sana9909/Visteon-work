/**
 * @file fault.h
 * @brief Fault manager interface.
 *
 * Provides functions for bitwise fault flag management and
 * per-fault counter tracking across cycles.
 *
 * Design: Each fault type has a unique bit position (defined in types.h)
 * and a dedicated counter. New fault types can be added by:
 *   1. Adding a new bit mask in types.h
 *   2. Adding a counter field in FaultStatus
 *   3. Extending increment_fault_counter() and update_fault_status()
 */

#ifndef FAULT_H_
#define FAULT_H_

#include "types.h"

/**
 * @brief Set a fault flag (bitwise OR).
 * @param faults     Pointer to FaultStatus.
 * @param fault_bit  Fault bit mask to set (e.g., FAULT_OVERSPEED).
 */
void set_fault(FaultStatus *faults, uint32_t fault_bit);

/**
 * @brief Clear a specific fault flag (bitwise AND NOT).
 * @param faults     Pointer to FaultStatus.
 * @param fault_bit  Fault bit mask to clear.
 */
void clear_fault(FaultStatus *faults, uint32_t fault_bit);

/**
 * @brief Clear all active fault flags (resets to 0).
 * Does NOT reset counters — those persist across cycles.
 * Called at the start of each scheduler cycle.
 * @param faults  Pointer to FaultStatus.
 */
void clear_all_faults(FaultStatus *faults);

/**
 * @brief Increment the counter for a specific fault type.
 * @param faults     Pointer to FaultStatus.
 * @param fault_bit  Fault bit mask identifying which counter to increment.
 */
void increment_fault_counter(FaultStatus *faults, uint32_t fault_bit);

/**
 * @brief Update fault counters based on currently active fault flags.
 * For each active fault bit, increments the corresponding counter.
 * Called once per cycle AFTER all detection modules have run.
 * @param faults  Pointer to FaultStatus.
 */
void update_fault_status(FaultStatus *faults);

#endif /* FAULT_H_ */
