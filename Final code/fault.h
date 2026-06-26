#ifndef FAULT_H
#define FAULT_H

#include "types.h"

/* ─── Function Prototypes ──────────────────────────────────── */

/**
 * set_fault()   - Set fault bit and increment counter.
 */
void set_fault(FaultStatus *faults, FaultType type);

/**
 * clear_fault() - Clear fault bit (counter is preserved for history).
 */
void clear_fault(FaultStatus *faults, FaultType type);

/**
 * update_fault_status() - Increment persistent counters for active faults,
 *                         reset counters for cleared faults,
 *                         set persistent_flags for faults ≥ PERSISTENT_THRESHOLD.
 */
void update_fault_status(FaultStatus *faults);

/**
 * fault_type_to_string() - Human-readable fault name.
 */
const char *fault_type_to_string(FaultType type);

/**
 * active_fault_count() - Returns number of currently active faults.
 */
int active_fault_count(const FaultStatus *faults);

/**
 * persistent_fault_count() - Returns number of persistent faults.
 */
int persistent_fault_count(const FaultStatus *faults);

#endif /* FAULT_H */