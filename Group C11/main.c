/**
 * @file main.c
 * @brief Scheduler loop and system integration — MISRA refactored.
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

/* Forward declarations — implemented in lib/ecu_control.c */
extern void run_all_test_cases(int16_t target_index, VehicleStatus *status, FaultStatus *faults);
extern uint16_t get_total_tests(void);
extern const char* get_test_name_by_index(uint16_t index);

int main(void)
{
    VehicleInput  input  = {0};
    VehicleStatus status = {0};
    FaultStatus   faults = {0};

    int16_t choice = 0;

    sys_printf("\n=== Vehicle ECU Simulator ===\n");
    sys_printf("0 = Interactive mode (manual input)\n");
    sys_printf("1 = Auto-run all mandatory test cases\n");
    sys_printf("2 = Run a specific test case\n");
    sys_printf("Choice: ");
    
    if (!sys_read_int16(&choice)) {
        sys_log_error("Invalid input for choice");
    }

    if (choice == 1) {
        /* Auto-run all: target_index = -1 */
        run_all_test_cases(-1, &status, &faults);
    } else if (choice == 2) {
        /* Selective run */
        uint16_t total = get_total_tests();
        if (total == 0) {
            sys_printf("No tests found in test_cases.json\n");
            return 1;
        }

        sys_printf("\nAvailable Test Cases:\n");
        for (uint16_t i = 0; i < total; i++) {
            sys_printf("%u: %s\n", (unsigned int)i, get_test_name_by_index(i));
        }
        
        int16_t test_idx = -1;
        sys_printf("\nEnter Test Index (0-%u): ", (unsigned int)(total - 1U));
        if (sys_read_int16(&test_idx) && test_idx >= 0 && (uint16_t)test_idx < total) {
            run_all_test_cases(test_idx, &status, &faults);
        } else {
            sys_printf("Invalid index. Aborting.\n");
        }
    } else {
        /* Interactive mode: initialize and show banner */
        init_system(&status, &faults);

        sys_printf("\nInteractive mode started. Ctrl+C to exit.\n\n");

        /* MISRA Rule 14.1: Loop shall be well-formed. Infinite loops in main are often allowed in embedded. */
        for (;;) {
            /* Step 1: Clear per-cycle fault flags (counters persist) */
            clear_all_faults(&faults);

            /* Step 2: Read raw inputs */
            read_inputs(&input);

            /* Step 3: Validate and sanitize inputs */
            validate_inputs(&input, &status, &faults);

            /* Step 4: Evaluate mode transition */
            update_mode(&status, &input, &faults);

            /* Step 5: Run control checks (overspeed, temp, gear) */
            run_control_checks(&input, &status, &faults);

            /* Step 6: Update fault counters */
            update_fault_status(&faults);

            /* Step 7: Evaluate system state (NORMAL/DEGRADED/SAFE) */
            evaluate_system_state(&status, &faults);

            /* Step 8: Generate cycle log */
            log_cycle_summary(&input, &status, &faults, NULL);
        }
    }

    return 0;
}
