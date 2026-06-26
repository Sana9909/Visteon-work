#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include "types.h"

/* ─── File Logger Control ──────────────────────────────────── */
void init_logger(const char *filename);
void close_logger(void);

/* Variadic logging APIs (printf-style) */
void log_info(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_fault(const char *fmt, ...);

/* Cycle summary logger */
void log_cycle_summary(const VehicleInput *input,
                       const VehicleStatus *status,
                       const FaultStatus *faults);

#endif /* LOG_H */