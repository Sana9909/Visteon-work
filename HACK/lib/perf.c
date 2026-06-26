/**
 * @file perf.c
 * @brief Performance measurement global state.
 */

#include "perf.h"

/* Global I/O suppression flag (0 = normal output, 1 = suppress diagnostics) */
volatile int g_suppress_io = 0;
