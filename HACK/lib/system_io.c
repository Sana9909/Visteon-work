/**
 * @file system_io.c
 * @brief Implementation of simulation I/O abstraction — MISRA refactored.
 */

#include "system_io.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

sys_file_t sys_fopen(const char *path, const char *mode)
{
    FILE *f = NULL;
    if ((path != NULL) && (mode != NULL)) {
        f = fopen(path, mode);
    }
    return (sys_file_t)f;
}

void sys_fclose(sys_file_t file)
{
    if (file != NULL) {
        (void)fclose((FILE*)file);
    }
}

size_t sys_fread(void *ptr, size_t size, size_t nmemb, sys_file_t stream)
{
    size_t read = 0U;
    if ((ptr != NULL) && (stream != NULL)) {
        read = fread(ptr, size, nmemb, (FILE*)stream);
    }
    return read;
}

char *sys_strncpy(char *dest, const char *src, size_t n)
{
    char *res = NULL;
    if ((dest != NULL) && (src != NULL)) {
        res = strncpy(dest, src, n);
    }
    return res;
}

int32_t sys_strncmp(const char *s1, const char *s2, size_t n)
{
    int32_t res = 0;
    if ((s1 != NULL) && (s2 != NULL)) {
        res = (int32_t)strncmp(s1, s2, n);
    }
    return res;
}

size_t sys_strlen(const char *s)
{
    size_t len = 0U;
    if (s != NULL) {
        len = strlen(s);
    }
    return len;
}

void sys_memset(void *ptr, int32_t value, size_t num)
{
    if (ptr != NULL) {
        (void)memset(ptr, (int)value, num);
    }
}

void sys_log_info(const char *msg)
{
    if (msg != NULL) {
        (void)printf("[INFO] %s\n", msg);
    }
}

void sys_log_error(const char *msg)
{
    if (msg != NULL) {
        (void)fprintf(stderr, "[ERROR] %s\n", msg);
    }
}

void sys_printf(const char *fmt, ...)
{
    va_list args;
    if (fmt != NULL) {
        va_start(args, fmt);
        (void)vprintf(fmt, args);
        va_end(args);
    }
}

void sys_fprintf(sys_file_t file, const char *fmt, ...)
{
    va_list args;
    if (fmt != NULL) {
        if (file != NULL) {
            va_start(args, fmt);
            (void)vfprintf((FILE*)file, fmt, args);
            va_end(args);
        } else {
            va_start(args, fmt);
            (void)vprintf(fmt, args);
            va_end(args);
        }
    }
}

bool sys_read_int16(int16_t *value)
{
    int temp = 0;
    bool success = false;
    if (value != NULL) {
        if (scanf("%d", &temp) == 1) {
            *value = (int16_t)temp;
            success = true;
        }
    }
    return success;
}

bool sys_read_int8(int8_t *value)
{
    int temp = 0;
    bool success = false;
    if (value != NULL) {
        if (scanf("%d", &temp) == 1) {
            *value = (int8_t)temp;
            success = true;
        }
    }
    return success;
}

bool sys_read_uint32(uint32_t *value)
{
    unsigned int temp = 0U;
    bool success = false;
    if (value != NULL) {
        if (scanf("%u", &temp) == 1) {
            *value = (uint32_t)temp;
            success = true;
        }
    }
    return success;
}

int32_t sys_atoi(const char *s)
{
    int32_t res = 0;
    if (s != NULL) {
        res = (int32_t)atoi(s);
    }
    return res;
}

void sys_get_timestamp(char *buf, size_t size)
{
    if ((buf != NULL) && (size > 0U)) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        if (tm_info != NULL) {
            (void)strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            (void)strncpy(buf, "UNKNOWN TIME", size - 1U);
            buf[size - 1U] = '\0';
        }
    }
}
