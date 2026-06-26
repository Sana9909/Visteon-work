/**
 * @file system_io.h
 * @brief MISRA-compliant I/O abstraction layer.
 */

#ifndef SYSTEM_IO_H_
#define SYSTEM_IO_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Opaque handle for file operations, isolating logic from <stdio.h>.
 */
typedef void* sys_file_t;

/**
 * @brief File management.
 */
sys_file_t sys_fopen(const char *path, const char *mode);
void sys_fclose(sys_file_t file);
size_t sys_fread(void *ptr, size_t size, size_t nmemb, sys_file_t stream);

/**
 * @brief String and memory abstractions.
 */
char *sys_strncpy(char *dest, const char *src, size_t n);
int32_t sys_strncmp(const char *s1, const char *s2, size_t n);
size_t sys_strlen(const char *s);
void sys_memset(void *ptr, int32_t value, size_t num);

/**
 * @brief Logging interface for simulation output.
 */
void sys_log_info(const char *msg);
void sys_log_error(const char *msg);

/**
 * @brief Formatted logging for internal simulation reports.
 */
void sys_printf(const char *fmt, ...);
void sys_fprintf(sys_file_t file, const char *fmt, ...);

/**
 * @brief Input abstraction for interactive mode.
 */
bool sys_read_int16(int16_t *value);
bool sys_read_int8(int8_t *value);
bool sys_read_uint32(uint32_t *value);

/**
 * @brief Numeric conversion abstractions (Rule 21.3 deviation for simulation).
 */
int32_t sys_atoi(const char *s);

/**
 * @brief Time and timestamping.
 */
void sys_get_timestamp(char *buf, size_t size);

/**
 * @brief Registration for persistent simulation logging.
 */
void sys_set_simulation_log(sys_file_t logfile);

/**
 * @brief Standardized logging for data clamping/correction.
 * Writes to console (if not suppressed) AND simulation log (if registered).
 */
void sys_log_clamping(const char *module, const char *param, int32_t val, int32_t fallback);

#endif /* SYSTEM_IO_H_ */
