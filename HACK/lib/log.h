/**
 * @file log.h
 * @brief Structured console logging for ECU simulation.
 */

#ifndef LOG_H_
#define LOG_H_

#include "types.h"
#include "system_io.h"
#include "perf.h"

/**
 * @brief Log high-level summary of one cycle.
 *
 * @param input    Pointer to VehicleInput (current cycle values).
 * @param status   Pointer to VehicleStatus (mode, state).
 * @param faults   Pointer to FaultStatus (flags, counters).
 * @param logfile  sys_file_t for file output (NULL = console only).
 */
void log_cycle_summary(const VehicleInput *input,
                       const VehicleStatus *status,
                       const FaultStatus *faults,
                       sys_file_t logfile);

/**
 * @brief Log CPU cycle timing for one scheduler cycle.
 *
 * @param logfile   sys_file_t for file output (NULL = console only).
 * @param cycle_num Cycle number within the current test case.
 * @param timing    Pointer to CycleTiming with measured values.
 */
void log_cycle_timing(sys_file_t logfile, uint16_t cycle_num, const CycleTiming *timing);

const char* mode_to_name(Mode m);
const char* state_to_name(SystemState s);
void fault_flags_to_csv_str(FaultFlags flags, char *buf, size_t size);

/**
 * @brief Convert human-readable names to Mode/State/Fault enums.
 */
Mode        name_to_mode(const char *name);
SystemState name_to_state(const char *name);
FaultFlags  name_to_fault(const char *name);


#endif /* LOG_H_ */
