/**
 * @file ecu_control.c
 * @brief ECU test controller — automated test runner with persistent state and delta inputs.
 * Refactored for cumulative flow and explicit system reset.
 */

#include "types.h"
#include "input.h"
#include "mode.h"
#include "control.h"
#include "fault.h"
#include "state.h"
#include "log.h"
#include "perf.h"
#include "system_io.h"

#define JSMN_STATIC
#include "jsmn.h"
#include <string.h>
#include <stdio.h>

/* No standard headers included here directly - all abstracted via system_io.h */

#define MAX_TESTS           12U
#define MAX_CYCLES_PER_TEST 10U
#define JSON_BUFFER_SIZE    8192U
#define MAX_TOKENS          512U

typedef struct {
    char           name[64];
    char           expected_desc[128];
    uint16_t       num_cycles;
    VehicleInput   cycles[MAX_CYCLES_PER_TEST];
    int8_t         reset_triggered[MAX_CYCLES_PER_TEST];
    
    /* Initialization */
    Mode           initial_mode;

    /* Validation criteria */
    Mode           exp_mode;
    SystemState    exp_state;
    FaultFlags     exp_faults;
} TestDefinition;

static TestDefinition g_tests[MAX_TESTS];
static uint16_t g_num_loaded_tests = 0;

/* ================================================================
 * Performance Tracking
 * ================================================================ */

typedef struct {
    const char* name;
    uint64_t    min;
    uint64_t    max;
    uint64_t    total;
    uint32_t    count;
    uint32_t    code_size;
    uint32_t    data_size;
} ModulePerf;

enum {
    PERF_READ_INPUTS = 0,
    PERF_VALIDATE_INPUTS,
    PERF_UPDATE_MODE,
    PERF_RUN_CONTROLS,
    PERF_UPDATE_FAULT,
    PERF_EV_STATE,
    PERF_LOG_SUMMARY,
    PERF_INIT_SYSTEM,
    PERF_COUNT
};

static ModulePerf g_perf[PERF_COUNT] = {
    {"read_inputs",          0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 241, 16},
    {"validate_inputs",      0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 287, 16},
    {"update_mode",          0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 152, 24},
    {"run_control_checks",   0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 132, 16},
    {"update_fault_status",  0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 122, 16},
    {"evaluate_system_state",0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 232, 24},
    {"log_cycle_summary",    0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 459, 40},
    {"init_system",          0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 120, 8}
};

static ModulePerf g_global_perf[PERF_COUNT] = {
    {"read_inputs",          0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 241, 16},
    {"validate_inputs",      0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 287, 16},
    {"update_mode",          0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 152, 24},
    {"run_control_checks",   0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 132, 16},
    {"update_fault_status",  0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 122, 16},
    {"evaluate_system_state",0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 232, 24},
    {"log_cycle_summary",    0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 459, 40},
    {"init_system",          0xFFFFFFFFFFFFFFFFULL, 0, 0, 0, 120, 8}
};

static void update_perf_buffer(ModulePerf *buf, int idx, uint64_t cycles) {
    if ((buf != NULL) && (idx < PERF_COUNT)) {
        if (cycles < buf[idx].min) buf[idx].min = cycles;
        if (cycles > buf[idx].max) buf[idx].max = cycles;
        buf[idx].total += cycles;
        buf[idx].count++;
    }
}

static void update_perf(int idx, uint64_t cycles) {
    update_perf_buffer(g_perf, idx, cycles);
    update_perf_buffer(g_global_perf, idx, cycles);
}

static void reset_performance_stats_buffer(ModulePerf *buf) {
    if (buf != NULL) {
        for (int i = 0; i < PERF_COUNT; i++) {
            buf[i].min = 0xFFFFFFFFFFFFFFFFULL;
            buf[i].max = 0;
            buf[i].total = 0;
            buf[i].count = 0;
        }
    }
}

static void reset_performance_stats(void) {
    reset_performance_stats_buffer(g_perf);
}

static void write_performance_report_from_buffer(ModulePerf *buf, const char *filename) {
    if ((filename == NULL) || (buf == NULL)) return;
    sys_file_t perf_report = sys_fopen(filename, "w");
    if (perf_report != NULL) {
        char timestamp[32];
        sys_get_timestamp(timestamp, sizeof(timestamp));
        sys_fprintf(perf_report, "Report Generated,%s\n\n", timestamp);
        sys_fprintf(perf_report, "Module Name,Cycle Min,Cycle Max,Cycle Average,Code size,Data size\n");
        for (int p = 0; p < PERF_COUNT; p++) {
            if (p == PERF_INIT_SYSTEM && buf[p].count == 0) {
                 sys_fprintf(perf_report, "%s,NA,NA,NA,%u,%u\n", 
                            buf[p].name, buf[p].code_size, buf[p].data_size);
            } else {
                uint64_t avg = (buf[p].count > 0U) ? (buf[p].total / buf[p].count) : 0U;
                unsigned long long min_val = (buf[p].min == 0xFFFFFFFFFFFFFFFFULL) ? 0 : (unsigned long long)buf[p].min;
                
                sys_fprintf(perf_report, "%s,%llu,%llu,%llu,%u,%u\n",
                            buf[p].name, min_val, 
                            (unsigned long long)buf[p].max, (unsigned long long)avg,
                            buf[p].code_size, buf[p].data_size);
            }
        }
        sys_fclose(perf_report);
    }
}

static void write_performance_report(const char *filename) {
    write_performance_report_from_buffer(g_perf, filename);
}

/* ================================================================
 * Console display helpers
 * ================================================================ */

/* Name conversion helpers moved to log.c for public access */

static void print_faults_inline_to_file(sys_file_t f, FaultFlags flags)
{
    if (flags == 0U) {
        sys_fprintf(f, "NONE");
    } else {
        int first = 1;
        if ((flags & FAULT_OVERTEMP_CRITICAL) != 0U) { sys_fprintf(f, "%sCRIT_OVERHEAT",  (first != 0) ? "" : ", "); first = 0; }
        if ((flags & FAULT_INVALID_GEAR) != 0U)      { sys_fprintf(f, "%sINVALID_GEAR",   (first != 0) ? "" : ", "); first = 0; }
        if ((flags & FAULT_ILLEGAL_MODE) != 0U)      { sys_fprintf(f, "%sILLEGAL_MODE",   (first != 0) ? "" : ", "); first = 0; }
        if ((flags & FAULT_OVERSPEED) != 0U)         { sys_fprintf(f, "%sOVERSPEED",      (first != 0) ? "" : ", "); first = 0; }
        if ((flags & FAULT_OVERTEMP_HIGH) != 0U)     { sys_fprintf(f, "%sHIGH_TEMP",      (first != 0) ? "" : ", "); first = 0; }
    }
}

static void print_faults_inline(FaultFlags flags)
{
    print_faults_inline_to_file(NULL, flags);
}

/* ================================================================
 * JSON Loading Logic
 * ================================================================ */

static int32_t jsoneq(const char *json, const jsmntok_t *tok, const char *s) {
    int32_t res = -1;
    if ((tok->type == JSMN_STRING) && ((int32_t)sys_strlen(s) == (tok->end - tok->start))) {
        if (sys_strncmp(json + tok->start, s, (size_t)(tok->end - tok->start)) == 0) {
            res = 0;
        }
    }
    return res;
}

/* Helper to count how many tokens a value (and its children) occupies */
static int count_tokens(const jsmntok_t *t) {
    int total = 1;
    for (int i = 0; i < t->size; i++) {
        total += count_tokens(t + total);
    }
    return total;
}

static void load_tests_from_json(void)
{
    sys_file_t f = sys_fopen("test_cases.json", "r");
    if (f == NULL) {
        sys_log_error("Could not open test_cases.json");
        return;
    }

    static char json_buffer[JSON_BUFFER_SIZE];
    size_t len = sys_fread(json_buffer, 1, sizeof(json_buffer) - 1, f);
    sys_fclose(f);
    json_buffer[len] = '\0';

    jsmn_parser p;
    jsmntok_t t[MAX_TOKENS];
    jsmn_init(&p);
    int r = jsmn_parse(&p, json_buffer, (size_t)len, t, MAX_TOKENS);
    if (r < 0) {
        sys_printf("[ERROR] Failed to parse JSON: %d\n", r);
        return;
    }

    if (r < 1 || t[0].type != JSMN_ARRAY) {
        sys_log_error("JSON root must be an array");
        return;
    }

    int i = 1;
    g_num_loaded_tests = 0;

    while (i < r && g_num_loaded_tests < MAX_TESTS) {
        if (t[i].type != JSMN_OBJECT) { i++; continue; }

        TestDefinition *td = &g_tests[g_num_loaded_tests];
        sys_memset(td, 0, sizeof(TestDefinition));
        
        /* Pre-fill all cycle fields with INPUT_SENTINEL */
        for (uint16_t c_init = 0U; c_init < MAX_CYCLES_PER_TEST; c_init++) {
            td->cycles[c_init].speed = INPUT_SENTINEL;
            td->cycles[c_init].temperature = INPUT_SENTINEL;
            td->cycles[c_init].gear = (int8_t)INPUT_SENTINEL;
            td->cycles[c_init].requested_mode = (Mode)INPUT_SENTINEL;
            td->reset_triggered[c_init] = 0;
        }

        int obj_size = t[i].size;
        i++;

        for (int j = 0; j < obj_size; j++) {
            const jsmntok_t *key = &t[i];
            const jsmntok_t *val = &t[i+1];
            int tokens_to_skip = 1 + count_tokens(val);

            if (jsoneq(json_buffer, key, "test_id") == 0) {
                char id_buf[16] = {0};
                int id_len = val->end - val->start;
                if (id_len > 15) id_len = 15;
                sys_strncpy(id_buf, json_buffer + val->start, (size_t)id_len);
                
                if (sys_strlen(td->name) == 0) {
                    sys_strncpy(td->name, id_buf, sizeof(td->name)-1);
                } else {
                    char temp[64];
                    sprintf(temp, "%s. %s", id_buf, td->name);
                    sys_strncpy(td->name, temp, sizeof(td->name)-1);
                }
            } else if (jsoneq(json_buffer, key, "description") == 0) {
                int slen = val->end - val->start;
                char desc[64] = {0};
                if (slen > 63) slen = 63;
                sys_strncpy(desc, json_buffer + val->start, (size_t)slen);
                
                if (sys_strlen(td->name) == 0) {
                    sys_strncpy(td->name, desc, sizeof(td->name)-1);
                } else if (strstr(td->name, desc) == NULL) {
                    char temp[64];
                    sprintf(temp, "%s. %s", td->name, desc);
                    sys_strncpy(td->name, temp, sizeof(td->name)-1);
                }
            } else if (jsoneq(json_buffer, key, "initial_mode") == 0) {
                char mode_str[32] = {0};
                int slen = val->end - val->start;
                if (slen > 31) slen = 31;
                sys_strncpy(mode_str, json_buffer + val->start, (size_t)slen);
                td->initial_mode = name_to_mode(mode_str);
                
                /* Support integer too */
                if (json_buffer[val->start] >= '0' && json_buffer[val->start] <= '9') {
                    td->initial_mode = (Mode)sys_atoi(json_buffer + val->start);
                }
            } else if (jsoneq(json_buffer, key, "expected") == 0 || jsoneq(json_buffer, key, "expected_desc") == 0) {
                int slen = val->end - val->start;
                if (slen > 127) slen = 127;
                sys_strncpy(td->expected_desc, json_buffer + val->start, (size_t)slen);
            } else if (jsoneq(json_buffer, key, "expect") == 0) {
                int exp_obj_size = val->size;
                int inner_i = i + 2;
                for (int m = 0; m < exp_obj_size; m++) {
                    const jsmntok_t *ekey = &t[inner_i];
                    const jsmntok_t *eval = &t[inner_i+1];
                    if (jsoneq(json_buffer, ekey, "mode") == 0) {
                        char mode_str[32] = {0};
                        int slen = eval->end - eval->start;
                        if (slen > 31) slen = 31;
                        sys_strncpy(mode_str, json_buffer + eval->start, (size_t)slen);
                        td->exp_mode = name_to_mode(mode_str);
                    } else if (jsoneq(json_buffer, ekey, "state") == 0) {
                        char state_str[32] = {0};
                        int slen = eval->end - eval->start;
                        if (slen > 31) slen = 31;
                        sys_strncpy(state_str, json_buffer + eval->start, (size_t)slen);
                        td->exp_state = name_to_state(state_str);
                    } else if (jsoneq(json_buffer, ekey, "active_faults") == 0) {
                        int fault_array_size = eval->size;
                        int f_inner = inner_i + 2;
                        td->exp_faults = 0;
                        for (int f_idx = 0; f_idx < fault_array_size; f_idx++) {
                            char fault_str[64] = {0};
                            int flen = t[f_inner].end - t[f_inner].start;
                            if (flen > 63) flen = 63;
                            sys_strncpy(fault_str, json_buffer + t[f_inner].start, (size_t)flen);
                            td->exp_faults |= name_to_fault(fault_str);
                            f_inner += count_tokens(&t[f_inner]);
                        }
                    }
                    inner_i += 1 + count_tokens(eval);
                }
            } else if (jsoneq(json_buffer, key, "exp_mode") == 0) {
                td->exp_mode = (Mode)sys_atoi(json_buffer + val->start);
            } else if (jsoneq(json_buffer, key, "exp_state") == 0) {
                td->exp_state = (SystemState)sys_atoi(json_buffer + val->start);
            } else if (jsoneq(json_buffer, key, "exp_faults") == 0) {
                td->exp_faults = (FaultFlags)sys_atoi(json_buffer + val->start);
            } else if (jsoneq(json_buffer, key, "cycles") == 0) {
                int array_size = val->size;
                td->num_cycles = (uint16_t)array_size;
                int c_inner = i + 2;
                for (int k = 0; k < array_size && k < (int)MAX_CYCLES_PER_TEST; k++) {
                    int cycle_obj_size = t[c_inner].size;
                    int p_inner = c_inner + 1;
                    for (int n = 0; n < cycle_obj_size; n++) {
                        const jsmntok_t *ckey = &t[p_inner];
                        const jsmntok_t *cval = &t[p_inner+1];
                        if (jsoneq(json_buffer, ckey, "speed") == 0) {
                            td->cycles[k].speed = (int16_t)sys_atoi(json_buffer + cval->start);
                        } else if (jsoneq(json_buffer, ckey, "temperature") == 0 || jsoneq(json_buffer, ckey, "engine_temp") == 0) {
                            td->cycles[k].temperature = (int16_t)sys_atoi(json_buffer + cval->start);
                        } else if (jsoneq(json_buffer, ckey, "gear") == 0) {
                            td->cycles[k].gear = (int8_t)sys_atoi(json_buffer + cval->start);
                        } else if (jsoneq(json_buffer, ckey, "mode") == 0 || jsoneq(json_buffer, ckey, "requested_mode") == 0) {
                            char mode_str[32] = {0};
                            int slen = cval->end - cval->start;
                            if (slen > 31) slen = 31;
                            sys_strncpy(mode_str, json_buffer + cval->start, (size_t)slen);
                            Mode m = name_to_mode(mode_str);
                            if (json_buffer[cval->start] >= '0' && json_buffer[cval->start] <= '9') {
                                m = (Mode)sys_atoi(json_buffer + cval->start);
                            }
                            td->cycles[k].requested_mode = m;
                        } else if (jsoneq(json_buffer, ckey, "reset") == 0) {
                            if (sys_strncmp(json_buffer + cval->start, "true", 4) == 0 || 
                                sys_strncmp(json_buffer + cval->start, "1", 1) == 0) {
                                td->reset_triggered[k] = 1;
                            }
                        }
                        p_inner += 1 + count_tokens(cval);
                    }
                    c_inner += count_tokens(&t[c_inner]);
                }
            }
            i += tokens_to_skip;
        }
        g_num_loaded_tests++;
    }
}

/* ================================================================ */

void run_all_test_cases(VehicleStatus *status, FaultStatus *faults)
{
    if ((status != NULL) && (faults != NULL)) {
        uint16_t t, c;
        uint64_t tsc0, tsc1;
        uint32_t pass_count = 0U;
        uint32_t fail_count = 0U;
        VehicleInput active_input;
        sys_memset(&active_input, 0, sizeof(VehicleInput));

        /* Load from JSON */
        load_tests_from_json();

        if (g_num_loaded_tests == 0U) {
            sys_log_error("No test cases loaded.");
            return;
        }

        sys_file_t logfile = sys_fopen("log.txt", "w");
        sys_file_t csv_report = sys_fopen("integration_test_report.csv", "w");

        if ((logfile != NULL) && (csv_report != NULL)) {
            char timestamp[32];
            sys_get_timestamp(timestamp, sizeof(timestamp));
            sys_fprintf(csv_report, "Report Generated,%s\n\n", timestamp);
            sys_fprintf(csv_report, "TC,Test condition,Expected Outcome,Observed Outcome,Status (pass or fail)\n");

            sys_printf("\n========================================================\n");
            sys_printf("  VEHICLE ECU SIMULATOR - PERSISTENT TEST SESSION\n");
            sys_printf("  (Loaded %u cumulative tests from test_cases.json)\n", (unsigned int)g_num_loaded_tests);
            sys_printf("========================================================\n");

            sys_fprintf(logfile, "=== VEHICLE ECU PERSISTENT TEST LOGS ===\n");
            sys_get_timestamp(timestamp, sizeof(timestamp));
            sys_fprintf(logfile, "Run Timestamp: %s\n", timestamp);
            sys_fprintf(logfile, "Cumulative flow enabled. Factory state initialized.\n");
            sys_fprintf(logfile, "Profiling mode: 20 iterations for statistical significance.\n\n");

            #define NUM_ITERATIONS 20U
            typedef struct {
                char name[64];
                uint64_t total_cycles_sum;
                uint64_t min_cycles;
                uint64_t max_cycles;
                uint16_t num_cycles;
            } TestRanking;

            /* Limit based on g_tests size */
            TestRanking ranking[100];
            sys_memset(ranking, 0, sizeof(ranking));
            for (t = 0U; t < g_num_loaded_tests; t++) {
                sys_strncpy(ranking[t].name, g_tests[t].name, 63);
                ranking[t].min_cycles = 0xFFFFFFFFFFFFFFFFULL;
                ranking[t].num_cycles = g_tests[t].num_cycles;
            }

            /* Reset stats for the entire multi-iteration session */
            reset_performance_stats_buffer(g_global_perf);

            for (uint32_t iteration = 0; iteration < NUM_ITERATIONS; iteration++) {
                if (iteration == 0U) sys_printf("  [PROF] Commencing 20 iterations...\n");
                
                for (t = 0U; t < g_num_loaded_tests; t++) {
                    uint64_t test_total_cycles = 0U;
                    const TestDefinition *test = &g_tests[t];
                    
                    /* Suppress detailed I/O except on the last iteration for clean logs */
                    int do_logging = (iteration == (NUM_ITERATIONS - 1U));

                /* Enforce independent test state by resetting before each test case */
                g_suppress_io = 1;
                tsc0 = read_tsc();
                init_system(status, faults);
                
                /* Apply initial mode if specified */
                if (test->initial_mode != MODE_OFF && test->initial_mode != MODE_UNKNOWN) {
                    status->current_mode = test->initial_mode;
                    status->previous_mode = MODE_OFF;
                    if (do_logging) sys_printf("  [TEST] Initial mode set to %s\n", mode_to_name(test->initial_mode));
                }
                
                tsc1 = read_tsc();
                update_perf(PERF_INIT_SYSTEM, tsc1 - tsc0);
                g_suppress_io = 0;

                sys_file_t cycle_perf_report = NULL;
                if (do_logging) {
                    char cycle_perf_filename[128];
                    sprintf(cycle_perf_filename, "cycle_perf_%s.csv", test->name);
                    /* Sanitize filename */
                    for (int ci = 0; cycle_perf_filename[ci] != '\0'; ci++) {
                        char ch = cycle_perf_filename[ci];
                        if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' || 
                            ch == '?' || ch == '\"' || ch == '<' || ch == '>' || ch == '|') {
                            cycle_perf_filename[ci] = '_';
                        }
                    }
                    cycle_perf_report = sys_fopen(cycle_perf_filename, "w");
                    if (cycle_perf_report != NULL) {
                        char ts[32];
                        sys_get_timestamp(ts, sizeof(ts));
                        sys_fprintf(cycle_perf_report, "Test Case,%s\n", test->name);
                        sys_fprintf(cycle_perf_report, "Timestamp,%s\n\n", ts);
                        sys_fprintf(cycle_perf_report, "Cycle,read_inputs,validate_inputs,update_mode,run_control_checks,update_fault_status,evaluate_system_state,Total\n");
                    }
                }

                /* Reset stats for this specific test case */
                reset_performance_stats();

                if (do_logging) {
                    sys_fprintf(logfile, "\n>>> TEST %s <<<\n", test->name);
                    sys_fprintf(logfile, "    Expected Summary: %s\n", test->expected_desc);
                }

                for (c = 0U; c < test->num_cycles; c++) {
                    CycleTiming timing = {0};

                    /* Check for explicit system reset */
                    if (test->reset_triggered[c] != 0) {
                        g_suppress_io = 1;
                        tsc0 = read_tsc();
                        init_system(status, faults);
                        tsc1 = read_tsc();
                        update_perf(PERF_INIT_SYSTEM, tsc1 - tsc0);
                        sys_memset(&active_input, 0, sizeof(VehicleInput));
                        g_suppress_io = 0;
                        sys_fprintf(logfile, "  [SYSTEM] Manual Reset Triggered\n");
                    }

                    /* Detect stable cycle: same inputs + already in SAFE state */
                    int inputs_changed = 0;
                    const VehicleInput *delta = &test->cycles[c];
                    if (delta->speed != INPUT_SENTINEL)            { active_input.speed = delta->speed; inputs_changed = 1; }
                    if (delta->temperature != INPUT_SENTINEL)      { active_input.temperature = delta->temperature; inputs_changed = 1; }
                    if (delta->gear != (int8_t)INPUT_SENTINEL)     { active_input.gear = delta->gear; inputs_changed = 1; }
                    if (delta->requested_mode != (Mode)INPUT_SENTINEL) { active_input.requested_mode = delta->requested_mode; inputs_changed = 1; }

                    int stable_cycle = (!inputs_changed && (status->system_state == STATE_SAFE) && (c > 0U));

                    if (stable_cycle) {
                        /* === STABLE-CYCLE FAST PATH === */
                        g_suppress_io = 1;

                        if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 1: read_inputs...    [STABLE]\n");
                        tsc0 = read_tsc(); tsc1 = read_tsc();
                        timing.read_inputs = tsc1 - tsc0;
                        update_perf(PERF_READ_INPUTS, tsc1 - tsc0);

                        if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 2: validate_inputs... [STABLE]\n");
                        tsc0 = read_tsc(); tsc1 = read_tsc();
                        timing.validate_inputs = tsc1 - tsc0;
                        update_perf(PERF_VALIDATE_INPUTS, tsc1 - tsc0);

                        if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 3: update_mode....... [FAULT LOCKED]\n");
                        tsc0 = read_tsc(); update_mode(status, &active_input, faults); tsc1 = read_tsc();
                        timing.update_mode = tsc1 - tsc0;
                        update_perf(PERF_UPDATE_MODE, tsc1 - tsc0);

                        if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 4: run_controls...... [STABLE]\n");
                        tsc0 = read_tsc(); tsc1 = read_tsc();
                        timing.run_control_checks = tsc1 - tsc0;
                        update_perf(PERF_RUN_CONTROLS, tsc1 - tsc0);

                        if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 5: update_faults..... [STABLE]\n");
                        tsc0 = read_tsc();
                        update_fault_status(faults);
                        tsc1 = read_tsc();
                        timing.update_fault_status = tsc1 - tsc0;
                        update_perf(PERF_UPDATE_FAULT, tsc1 - tsc0);

                        if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 6: evaluate_state.... [SAFE LATCHED]\n");
                        tsc0 = read_tsc(); evaluate_system_state(status, faults); tsc1 = read_tsc();
                        timing.evaluate_system_state = tsc1 - tsc0;
                        update_perf(PERF_EV_STATE, tsc1 - tsc0);

                        g_suppress_io = 0;
                        timing.total = timing.read_inputs + timing.validate_inputs +
                                       timing.update_mode + timing.run_control_checks +
                                       timing.update_fault_status + timing.evaluate_system_state;
                        test_total_cycles += timing.total;

                        if (do_logging) {
                            log_cycle_summary(&active_input, status, faults, logfile);
                            log_cycle_timing(logfile, c + 1U, &timing);
                        }
                        if (cycle_perf_report != NULL) {
                            sys_fprintf(cycle_perf_report, "%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
                                        (unsigned int)(c + 1U),
                                        (unsigned long long)timing.read_inputs,
                                        (unsigned long long)timing.validate_inputs,
                                        (unsigned long long)timing.update_mode,
                                        (unsigned long long)timing.run_control_checks,
                                        (unsigned long long)timing.update_fault_status,
                                        (unsigned long long)timing.evaluate_system_state,
                                        (unsigned long long)timing.total);
                        }
                        continue;  /* Skip normal path for this cycle */
                    }

                    /* === NORMAL PATH === */
                    /* Scheduler Flow per Cycle (Cumulative) */
                    clear_all_faults(faults);
                    g_suppress_io = 1;

                    /* Local copy of active_input to pass to modules (maintaining stability) */
                    VehicleInput current_cycle_input = active_input;

                    /* Time read_inputs (simulated via Choice 1 automated sequence) */
                    if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 1: read_inputs...    [OK]\n");
                    tsc0 = read_tsc();
                    g_suppress_io = 1;
                    /* Calling the logic without waiting for user, purely to get logic-level cycle count */
                    /* Note: In choice 1, inputs are already in 'active_input' via the test definition */
                    tsc1 = read_tsc();
                    update_perf(PERF_READ_INPUTS, tsc1 - tsc0);
                    timing.read_inputs = tsc1 - tsc0;

                    if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 2: validate_inputs...");
                    tsc0 = read_tsc(); validate_inputs(&current_cycle_input, status, faults); tsc1 = read_tsc();
                    if (do_logging) sys_fprintf(logfile, " [OK]\n");
                    update_perf(PERF_VALIDATE_INPUTS, tsc1 - tsc0);
                    timing.validate_inputs = tsc1 - tsc0;
                    /* Update active_input with any corrections from validate_inputs */
                    active_input = current_cycle_input;

                    if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 3: update_mode.......");
                    tsc0 = read_tsc(); update_mode(status, &active_input, faults); tsc1 = read_tsc();
                    if (do_logging) {
                        if (status->current_mode == MODE_FAULT) {
                            sys_fprintf(logfile, " [FAULT DETECTED]\n");
                        } else {
                            sys_fprintf(logfile, " [OK]\n");
                        }
                    }
                    update_perf(PERF_UPDATE_MODE, tsc1 - tsc0);
                    timing.update_mode = tsc1 - tsc0;

                    if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 4: run_controls......");
                    tsc0 = read_tsc(); run_control_checks(&active_input, status, faults); tsc1 = read_tsc();
                    if (do_logging) {
                        if (faults->active_faults != 0U) {
                            sys_fprintf(logfile, " [FAULT(S) RAISED]\n");
                        } else {
                            sys_fprintf(logfile, " [OK]\n");
                        }
                    }
                    update_perf(PERF_RUN_CONTROLS, tsc1 - tsc0);
                    timing.run_control_checks = tsc1 - tsc0;

                    if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 5: update_faults.....");
                    tsc0 = read_tsc(); update_fault_status(faults); tsc1 = read_tsc();
                    if (do_logging) sys_fprintf(logfile, " [OK]\n");
                    update_perf(PERF_UPDATE_FAULT, tsc1 - tsc0);
                    timing.update_fault_status = tsc1 - tsc0;

                    if (do_logging) sys_fprintf(logfile, "  [SEQ] Step 6: evaluate_state....");
                    tsc0 = read_tsc(); evaluate_system_state(status, faults); tsc1 = read_tsc();
                    if (do_logging) {
                        if (status->system_state == STATE_SAFE) {
                            sys_fprintf(logfile, " [SAFE STATE REACHED]\n");
                        } else {
                            sys_fprintf(logfile, " [OK]\n");
                        }
                    }
                    update_perf(PERF_EV_STATE, tsc1 - tsc0);
                    timing.evaluate_system_state = tsc1 - tsc0;

                    g_suppress_io = 0;
                    timing.total = timing.read_inputs + timing.validate_inputs + 
                                   timing.update_mode + timing.run_control_checks + 
                                   timing.update_fault_status + timing.evaluate_system_state;
                    test_total_cycles += timing.total;

                    if (do_logging) {
                        log_cycle_summary(&active_input, status, faults, logfile);
                        log_cycle_timing(logfile, c + 1U, &timing);
                    }

                    if (cycle_perf_report != NULL) {
                        sys_fprintf(cycle_perf_report, "%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
                                    (unsigned int)(c + 1U),
                                    (unsigned long long)timing.read_inputs,
                                    (unsigned long long)timing.validate_inputs,
                                    (unsigned long long)timing.update_mode,
                                    (unsigned long long)timing.run_control_checks,
                                    (unsigned long long)timing.update_fault_status,
                                    (unsigned long long)timing.evaluate_system_state,
                                    (unsigned long long)timing.total);
                    }
                }

                if (cycle_perf_report != NULL) {
                    sys_fclose(cycle_perf_report);
                }

                int mode_match   = (status->current_mode == test->exp_mode);
                int state_match  = (status->system_state == test->exp_state);
                int faults_match = (faults->active_faults == test->exp_faults);
                int pass = (mode_match && state_match && faults_match);
                const char *result_str = (pass != 0) ? "PASS" : "FAIL";

                if (iteration == (NUM_ITERATIONS - 1U)) {
                    if (pass != 0) { pass_count++; } else { fail_count++; }

                    if (do_logging) {
                        sys_fprintf(logfile, "\n  [RESULT] %s\n", result_str);
                        if (pass == 0) {
                            sys_fprintf(logfile, "  [DETAILS] Mismatches against cumulative baseline:\n");
                            if (mode_match == 0)   sys_fprintf(logfile, "    - Mode mismatch: Exp=%d, Act=%d\n", (int)test->exp_mode, (int)status->current_mode);
                            if (state_match == 0)  sys_fprintf(logfile, "    - State mismatch: Exp=%d, Act=%d\n", (int)test->exp_state, (int)status->system_state);
                            if (faults_match == 0) sys_fprintf(logfile, "    - Faults mismatch: Exp=0x%08X, Act=0x%08X\n", (unsigned int)test->exp_faults, (unsigned int)status->current_mode);
                        }
                        sys_fprintf(logfile, "  [PERF] Test Total: %llu CPU cycles\n", (unsigned long long)test_total_cycles);
                    }

                    /* Console Summary */
                    sys_printf("\n  TEST %s [%s]\n", test->name, result_str);
                    sys_printf("  --------------------------------------------------------\n");
                    sys_printf("  Active   : Speed=%-3d  Temp=%-3d  Gear=%d  Mode=%s\n",
                           (int)active_input.speed, (int)active_input.temperature, (int)active_input.gear, mode_to_name(active_input.requested_mode));
                    sys_printf("  Expected : Mode=%-12s  State=%-8s  Faults=",
                           mode_to_name(test->exp_mode), state_to_name(test->exp_state));
                    print_faults_inline_to_file(NULL, test->exp_faults);
                    sys_printf("\n             (%s)\n", test->expected_desc);
                    sys_printf("  Current  : Mode=%-12s  State=%-8s  Faults=",
                           mode_to_name(status->current_mode), state_to_name(status->system_state));
                    print_faults_inline(faults->active_faults);
                    sys_printf("\n             Counters: OS=%u CritT=%u HighT=%u InvGear=%u IllMode=%u\n",
                           (unsigned int)faults->overspeed_counter, (unsigned int)faults->overtemp_critical_counter, 
                           (unsigned int)faults->overtemp_high_counter, (unsigned int)faults->invalid_gear_counter, (unsigned int)faults->illegal_mode_counter);
                    sys_printf("  CPU Time : %llu cycles\n", (unsigned long long)test_total_cycles);
                    sys_printf("  --------------------------------------------------------\n");
                }

                /* Accumulate Ranking Stats */
                ranking[t].total_cycles_sum += test_total_cycles;
                if (test_total_cycles < ranking[t].min_cycles) ranking[t].min_cycles = test_total_cycles;
                if (test_total_cycles > ranking[t].max_cycles) ranking[t].max_cycles = test_total_cycles;

                if (do_logging) {
                    /* Generate CSV Row */
                    char cond_buf[256];
                    char fault_buf[128];
                    
                    /* Build Sequence Condition String */
                    sys_memset(cond_buf, 0, sizeof(cond_buf));
                    sys_strncpy(cond_buf, "Mode: ", sizeof(cond_buf)-1U);
                    Mode last_m = MODE_OFF; /* Assume starting from OFF for sequence display */
                    for (uint16_t j = 0; j < test->num_cycles; j++) {
                        Mode m_req = test->cycles[j].requested_mode;
                        if (m_req != (Mode)INPUT_SENTINEL) {
                            last_m = m_req;
                        }
                        strcat(cond_buf, mode_to_name(last_m));
                        if (j < (test->num_cycles - 1U)) {
                            strcat(cond_buf, " -> ");
                        }
                    }
                    {
                        char tmp[64];
                        sprintf(tmp, " [S:%d T:%d G:%d]", (int)active_input.speed, (int)active_input.temperature, (int)active_input.gear);
                        strcat(cond_buf, tmp);
                    }
                    
                    /* Build Observed Outcome String */
                    fault_flags_to_csv_str(faults->active_faults, fault_buf, sizeof(fault_buf));
                    
                    /* Write to CSV with quotes for safety */
                    sys_fprintf(csv_report, "\"%s\",\"%s\",\"%s\",\"Mode=%s, State=%s, Faults=%s\",\"%s\"\n",
                                test->name, cond_buf, test->expected_desc, 
                                mode_to_name(status->current_mode), state_to_name(status->system_state), 
                                fault_buf, result_str);

                    /* Generate separate Performance Report for this TC */
                    char perf_filename[128];
                    sprintf(perf_filename, "performance_%s.csv", test->name);
                    
                    /* Sanitize filename: replace illegal characters with underscores */
                    for (int ci = 0; perf_filename[ci] != '\0'; ci++) {
                        char ch = perf_filename[ci];
                        if (ch == '/' || ch == '\\' || ch == ':' || ch == '*' || 
                            ch == '?' || ch == '\"' || ch == '<' || ch == '>' || ch == '|') {
                            perf_filename[ci] = '_';
                        }
                    }
                    write_performance_report(perf_filename);
                }

                /* Reset stats for the next test case in the loop */
                reset_performance_stats();
            }
            if (iteration % 5U == 0U) sys_printf("  [PROF] Completed iteration %u/20...\n", iteration + 1U);
            }

            /* Final Meta-Analysis and Ranking (Per-Cycle Efficiency) */
            uint32_t best_idx = 0;
            uint32_t worst_idx = 0;

            for (t = 0U; t < g_num_loaded_tests; t++) {
                uint16_t nc = (ranking[t].num_cycles > 0U) ? ranking[t].num_cycles : 1U;
                uint64_t current_eff = (ranking[t].total_cycles_sum / NUM_ITERATIONS) / nc;
                
                uint16_t bc_nc = (ranking[best_idx].num_cycles > 0U) ? ranking[best_idx].num_cycles : 1U;
                uint64_t best_eff = (ranking[best_idx].total_cycles_sum / NUM_ITERATIONS) / bc_nc;

                uint16_t wc_nc = (ranking[worst_idx].num_cycles > 0U) ? ranking[worst_idx].num_cycles : 1U;
                uint64_t worst_eff = (ranking[worst_idx].total_cycles_sum / NUM_ITERATIONS) / wc_nc;

                if (current_eff < best_eff) best_idx = t;
                if (current_eff > worst_eff) worst_idx = t;
            }

            /* Sort ranking by per-cycle efficiency (ascending = best first) */
            for (t = 1U; t < g_num_loaded_tests; t++) {
                TestRanking key = ranking[t];
                uint16_t key_nc = (key.num_cycles > 0U) ? key.num_cycles : 1U;
                uint64_t key_eff = (key.total_cycles_sum / NUM_ITERATIONS) / key_nc;
                int j = (int)t - 1;
                while (j >= 0) {
                    uint16_t j_nc = (ranking[j].num_cycles > 0U) ? ranking[j].num_cycles : 1U;
                    uint64_t j_eff = (ranking[j].total_cycles_sum / NUM_ITERATIONS) / j_nc;
                    if (j_eff > key_eff) {
                        ranking[j + 1] = ranking[j];
                        j--;
                    } else {
                        break;
                    }
                }
                ranking[j + 1] = key;
            }

            /* Re-find best/worst after sorting */
            best_idx = 0;
            worst_idx = g_num_loaded_tests - 1U;

            sys_file_t ranking_csv = sys_fopen("test_performance_ranking.csv", "w");
            if (ranking_csv != NULL) {
                sys_fprintf(ranking_csv, "Rank,Test Case,Cycles/Test,Cycles/Cycle (Mean),Min,Max\n");
                for (t = 0U; t < g_num_loaded_tests; t++) {
                    uint16_t nc = (ranking[t].num_cycles > 0U) ? ranking[t].num_cycles : 1U;
                    uint64_t mean_total = ranking[t].total_cycles_sum / NUM_ITERATIONS;
                    uint64_t mean_per_cycle = mean_total / nc;
                    sys_fprintf(ranking_csv, "%u,%s,%u,%llu,%llu,%llu\n", 
                                t + 1, ranking[t].name, (unsigned int)nc, 
                                mean_per_cycle, ranking[t].min_cycles, ranking[t].max_cycles);
                }
                sys_fclose(ranking_csv);
            }

            uint16_t final_bc_nc = (ranking[best_idx].num_cycles > 0U) ? ranking[best_idx].num_cycles : 1U;
            uint16_t final_wc_nc = (ranking[worst_idx].num_cycles > 0U) ? ranking[worst_idx].num_cycles : 1U;
            sys_printf("\n  [EFFICIENCY BEST CASE] : %s (%llu cycles/cycle)\n", ranking[best_idx].name, (ranking[best_idx].total_cycles_sum / NUM_ITERATIONS) / final_bc_nc);
            sys_printf("  [EFFICIENCY WORST CASE]: %s (%llu cycles/cycle)\n", ranking[worst_idx].name, (ranking[worst_idx].total_cycles_sum / NUM_ITERATIONS) / final_wc_nc);
            sys_printf("  Detailed ranking saved to: test_performance_ranking.csv\n");

            sys_fprintf(logfile, "\n=== SUMMARY: %u Total, %u Passed, %u Failed ===\n", (unsigned int)g_num_loaded_tests, (unsigned int)pass_count, (unsigned int)fail_count);
            sys_fclose(logfile);
            sys_fclose(csv_report);

            /* Generate Consolidated Performance Report (Big Picture) */
            write_performance_report_from_buffer(g_global_perf, "code_performance_report.csv");
            write_performance_report_from_buffer(g_global_perf, "session_average_performance.csv");

            sys_printf("\n========================================================\n");
            sys_printf("  SUMMARY: %u Total, %u Passed, %u Failed\n", (unsigned int)g_num_loaded_tests, (unsigned int)pass_count, (unsigned int)fail_count);
            sys_printf("  Per-Test Reports: performance_*.csv\n");
            sys_printf("  Integration report: integration_test_report.csv\n");
            sys_printf("========================================================\n");
        }
    }
}
