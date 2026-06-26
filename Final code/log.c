#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h> /* For adding timestamps to the log file */
#include "log.h"

#define TABLE_WIDTH  62

/* Global file pointer for the ECU black box */
static FILE *log_file = NULL;

/* ------------------------------------------------------------------
 * Logger Initialization
 * ------------------------------------------------------------------ */
void init_logger(const char *filename)
{
    /* Open in "a" (append) mode so we don't delete old test runs */
    log_file = fopen(filename, "a");
    if (log_file) {
        /* Get current time for a nice log header */
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        fprintf(log_file, "\n================ RUN START: %04d-%02d-%02d %02d:%02d:%02d ================\n",
                t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                t->tm_hour, t->tm_min, t->tm_sec);
        fflush(log_file);
    } else {
        printf("[WARN ] Could not open %s for writing.\n", filename);
    }
}

void close_logger(void)
{
    if (log_file) {
        fprintf(log_file, "================ RUN END ===================\n\n");
        fclose(log_file);
        log_file = NULL;
    }
}

/* ------------------------------------------------------------------
 * Internal helpers (Updated for Dual-Writing)
 * ------------------------------------------------------------------ */
static void print_line(void)
{
    int i;
    char buf[128] = {0};
    
    /* Build the line string once */
    buf[0] = '+';
    for (i = 1; i <= TABLE_WIDTH; i++) buf[i] = '-';
    buf[TABLE_WIDTH + 1] = '+';
    buf[TABLE_WIDTH + 2] = '\0';

    printf("%s\n", buf);
    if (log_file) fprintf(log_file, "%s\n", buf);
}

static void print_header(const char *title, int cycle)
{
    char header[64];
    snprintf(header, sizeof(header), "CYCLE %d  %s", cycle, title);
    
    print_line();
    printf("| %-60s |\n", header);
    if (log_file) fprintf(log_file, "| %-60s |\n", header);
    print_line();
}

static void print_kv(const char *key, const char *value)
{
    printf("| %-16s : %-39s |\n", key, value);
    if (log_file) fprintf(log_file, "| %-16s : %-39s |\n", key, value);
}

/* ------------------------------------------------------------------
 * Variadic logging APIs (Updated for Dual-Writing)
 * ------------------------------------------------------------------ */
static void log_vprint(const char *tag, const char *fmt, va_list args)
{
    /* We need a copy of the arguments because vprintf consumes them */
    va_list args_copy;
    va_copy(args_copy, args);

    /* Print to Terminal */
    printf("[%s] ", tag);
    vprintf(fmt, args);
    printf("\n");

    /* Print to File */
    if (log_file) {
        fprintf(log_file, "[%s] ", tag);
        vfprintf(log_file, fmt, args_copy);
        fprintf(log_file, "\n");
        fflush(log_file); /* Force save immediately in case of crash */
    }
    
    va_end(args_copy);
}

void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_vprint("INFO ", fmt, args);
    va_end(args);
}

void log_warning(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_vprint("WARN ", fmt, args);
    va_end(args);
}

void log_fault(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_vprint("FAULT", fmt, args);
    va_end(args);
}

/* ------------------------------------------------------------------
 * Cycle summary
 * ------------------------------------------------------------------ */
void log_cycle_summary(const VehicleInput *input,
                       const VehicleStatus *status,
                       const FaultStatus *faults)
{
    char buf[64];

    print_header("SUMMARY", status->cycle_count);

    snprintf(buf, sizeof(buf), "%d km/h", input->speed);
    print_kv("Speed", buf);

    int display_rpm = 800;
    if (status->current_mode == MODE_IGNITION_ON && input->gear > 0 && input->gear <= 5) {
        int multipliers[] = {0, 120, 70, 50, 35, 25};
        display_rpm = input->speed * multipliers[input->gear];
    } else if (status->current_mode != MODE_IGNITION_ON) {
        display_rpm = 0;
    }
    snprintf(buf, sizeof(buf), "%d RPM", display_rpm);
    print_kv("Engine Speed", buf);

    snprintf(buf, sizeof(buf), "%d C", input->temperature);
    print_kv("Temperature", buf);

    snprintf(buf, sizeof(buf), "%d", input->gear);
    print_kv("Gear", buf);

    snprintf(buf, sizeof(buf), "%s",
             status->current_mode == MODE_OFF ? "OFF" :
             status->current_mode == MODE_ACC ? "ACC" :
             status->current_mode == MODE_IGNITION_ON ? "IGNITION_ON" :
             "FAULT");
    print_kv("Mode", buf);

    snprintf(buf, sizeof(buf), "%s",
             status->system_state == STATE_NORMAL ? "NORMAL" :
             status->system_state == STATE_DEGRADED ? "DEGRADED" :
             "SAFE");
    print_kv("Sys State", buf);

    print_line();

    snprintf(buf, sizeof(buf), "0x%02X", faults->fault_flags);
    print_kv("Active Faults", buf);

    snprintf(buf, sizeof(buf), "0x%02X", faults->persistent_flags);
    print_kv("Persistent", buf);

    print_line();

    static const char *fault_labels[] = {
        "OVERSPEED",
        "OVERTEMP",
        "INVALID_GEAR",
        "INVALID_MODE",
        "REDLINE",
        "STALL"
    };

    for (int i = 0; i < FAULT_COUNT; i++) {
        snprintf(buf, sizeof(buf),
                 "cnt=%u  persist=%u",
                 faults->fault_counters[i],
                 faults->persistent_cycles[i]);
        print_kv(fault_labels[i], buf);
    }

    print_line();
    printf("\n");
    if (log_file) {
        fprintf(log_file, "\n");
        fflush(log_file);
    }
}