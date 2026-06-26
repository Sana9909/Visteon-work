/**
 * @file perf.h
 * @brief Performance measurement utilities.
 *
 * Provides CPU cycle counting via x86 RDTSC instruction
 * and a global I/O suppression flag to exclude printf overhead
 * from timing measurements.
 */

#ifndef PERF_H_
#define PERF_H_

#include <stdint.h>

/* ---- I/O Suppression ----
 * When set to 1, diagnostic printf calls in modules are suppressed.
 * This ensures CPU cycle measurements are not polluted by I/O latency.
 * Only affects diagnostic output in validate_inputs, update_mode,
 * and evaluate_system_state. Does NOT affect log_cycle_summary. */
extern volatile int g_suppress_io;

/* ---- CPU Cycle Timing Structure ---- */
typedef struct {
    uint64_t read_inputs;
    uint64_t validate_inputs;
    uint64_t update_mode;
    uint64_t run_control_checks;
    uint64_t update_fault_status;
    uint64_t evaluate_system_state;
    uint64_t total;
} CycleTiming;

/**
 * @brief Read the CPU Time Stamp Counter (TSC).
 *
 * Uses the x86 RDTSC instruction to get a monotonically increasing
 * cycle counter. Resolution is 1 CPU clock cycle.
 *
 * __asm__ __volatile__ is used (GCC reserved identifiers) to ensure
 * compatibility with -std=c99 -pedantic.
 *
 * @return Current TSC value (64-bit CPU cycle count).
 */
static inline uint64_t read_tsc(void)
{
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32U) | (uint64_t)lo;
}

#endif /* PERF_H_ */
