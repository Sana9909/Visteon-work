#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "types.h"
#include "input.h"
#include "mode.h"
#include "control.h"
#include "fault.h"
#include "state.h"
#include "log.h"
#include "test_loader.h"

/* ─── Configuration ────────────────────────────────────────── */
#define MAX_TEST_CASES  50
#define TEST_CASE_FILE "testcases.txt"

/* ─── System Init ──────────────────────────────────────────── */
static void init_system(VehicleStatus *status, FaultStatus *faults)
{
    memset(status, 0, sizeof(*status));
    memset(faults, 0, sizeof(*faults));
    status->current_mode  = MODE_OFF;
    status->previous_mode = MODE_OFF;
    status->system_state  = STATE_NORMAL;
    init_last_valid();
    log_info("System initialised. State set to NORMAL.");
}

/* ─── Scenario Execution (File Mode) ───────────────────────── */
static void run_scenario(const TestScenario *tc,
                         VehicleInput       *input,
                         VehicleStatus      *status,
                         FaultStatus        *faults)
{
    int r;

    printf("\n##################################################\n");
    printf("## SCENARIO: %s\n", tc->name);
    printf("##################################################\n");

    for (r = 0; r < tc->repeat; r++) {
        status->cycle_count++;

        /* 1. Read Inputs (from file struct) */
        input->speed          = tc->speed;
        input->temperature    = tc->temperature;
        input->gear           = tc->gear;
        input->requested_mode = tc->mode;
        input->input_valid    = 1;

        /* 2. Validate Inputs */
        validate_inputs(input, status);

        /* 3. Update Mode */
        update_mode(status, input, faults);

        /* --- THE RESET CONDITION (Limp Mode Escape) --- */
        if (status->current_mode == MODE_OFF && status->previous_mode != MODE_OFF) {
            log_info("Key Cycle Detected. Performing Hard ECU Reset...");
            reset_system_state(status);
            
            /* Wipe persistent memory on reboot */
            int j;
            for(j = 0; j < FAULT_COUNT; j++) {
                faults->persistent_cycles[j] = 0;
                CLEAR_BIT(faults->persistent_flags, j);
            }
        }

        /* 4. Control Checks */
        run_control_checks(input, status, faults);

        /* 5. Fault Manager */
        update_fault_status(faults);

        /* 6. System State */
        evaluate_system_state(status, faults);

        /* 7. Logging */
        log_cycle_summary(input, status, faults);
    }
}

/* ─── Entry Point ──────────────────────────────────────────── */
int main(void)
{
    VehicleInput  input  = {0};
    VehicleStatus status = {0};
    FaultStatus   faults = {0};

    TestScenario test_cases[MAX_TEST_CASES];
    int num_tests, i;
    int sim_mode = 0;

    /* Initialize dual-logging to terminal and ecu_logs.txt */
    init_logger("ecu_logs.txt");

    init_system(&status, &faults);

    /* --- Interactive Startup Menu --- */
    printf("\n========================================\n");
    printf("   VEHICLE ECU SIMULATOR\n");
    printf("========================================\n");
    printf("Select Execution Mode:\n");
    printf("1. File Test Mode (Load from testcases.txt)\n");
    printf("2. Interactive Terminal Mode (Manual Input)\n");
    printf("Choice (1 or 2): ");
    
    if (scanf("%d", &sim_mode) != 1) {
        sim_mode = 1; /* Default to 1 if input is mangled */
    }

    /* --- Branch 1: File Mode --- */
    if (sim_mode == 1) {
        printf("\n--- RUNNING FILE TEST MODE ---\n");
        num_tests = load_test_cases(TEST_CASE_FILE, test_cases, MAX_TEST_CASES);
        if (num_tests == 0) {
            log_warning("No test cases executed. Check testcases.txt format.");
            close_logger();
            return 1;
        }

        for (i = 0; i < num_tests; i++) {
            run_scenario(&test_cases[i], &input, &status, &faults);
        }

        printf("\n========================================\n");
        printf("   ALL TEST CASES COMPLETED\n");
        printf("========================================\n");
    } 
    /* --- Branch 2: Interactive Terminal Mode --- */
    else if (sim_mode == 2) {
        printf("\n--- RUNNING INTERACTIVE MODE ---\n");
        printf("Press Ctrl+C to exit the simulator at any time.\n");
        
        while (1) {
            status.cycle_count++;

            /* 1. Read Inputs (from user via terminal) */
            read_inputs(&input);

            /* 2. Validate Inputs */
            validate_inputs(&input, &status);

            /* 3. Update Mode */
            update_mode(&status, &input, &faults);

            /* --- THE RESET CONDITION (Limp Mode Escape) --- */
            if (status.current_mode == MODE_OFF && status.previous_mode != MODE_OFF) {
                log_info("Key Cycle Detected. Performing Hard ECU Reset...");
                reset_system_state(&status);
                
                int j;
                for(j = 0; j < FAULT_COUNT; j++) {
                    faults.persistent_cycles[j] = 0;
                    CLEAR_BIT(faults.persistent_flags, j);
                }
            }

            /* 4. Control Checks */
            run_control_checks(&input, &status, &faults);

            /* 5. Fault Manager */
            update_fault_status(&faults);

            /* 6. System State */
            evaluate_system_state(&status, &faults);

            /* 7. Logging */
            log_cycle_summary(&input, &status, &faults);
        }
    } 
    /* --- Invalid Selection --- */
    else {
        printf("\nInvalid selection. Exiting Simulator.\n");
    }

    /* Close the log file gracefully before exiting */
    close_logger();

    return 0;
}