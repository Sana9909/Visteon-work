/**
 * @file input.c
 * @brief Input handling and validation implementation — MISRA refactored.
 */

#include "input.h"
#include "fault.h"
#include "perf.h"
#include "system_io.h"

void read_inputs(VehicleInput *input)
{
    if (input != NULL) {
        int16_t temp_i16 = 0;
        int8_t  temp_i8  = 0;

        sys_printf("\n--- New ECU Cycle ---\n");

        sys_printf("Enter speed (0-200): ");
        if (!sys_read_int16(&temp_i16)) {
            sys_log_error("Invalid input for speed");
        }
        input->speed = temp_i16;

        sys_printf("Enter temperature (-40 to 150): ");
        if (!sys_read_int16(&temp_i16)) {
            sys_log_error("Invalid input for temperature");
        }
        input->temperature = temp_i16;

        sys_printf("Enter gear (0-5): ");
        if (!sys_read_int8(&temp_i8)) {
            sys_log_error("Invalid input for gear");
        }
        input->gear = temp_i8;

        sys_printf("Enter requested mode (0=OFF, 1=ACC, 2=IGNITION_ON, 3=FAULT): ");
        if (!sys_read_int16(&temp_i16)) {
            sys_log_error("Invalid input for mode");
        }
        
        /* MISRA: Assigning integer to enum requires validation and explicit cast */
        if ((temp_i16 >= (int16_t)MODE_MIN) && (temp_i16 <= (int16_t)MODE_MAX)) {
            input->requested_mode = (Mode)temp_i16;
        } else {
            input->requested_mode = MODE_UNKNOWN;
        }
    }
}

void validate_inputs(VehicleInput *input, VehicleStatus *status, FaultStatus *faults)
{
    if ((input != NULL) && (status != NULL) && (faults != NULL)) {
        /* ---- Stability Bypass: skip validation if inputs unchanged ---- */
        if ((input->speed == status->last_valid_speed) &&
            (input->temperature == status->last_valid_temperature) &&
            (input->gear == status->last_valid_gear) &&
            (input->requested_mode == status->last_valid_mode)) {
            return;  /* inputs stable — no re-validation needed */
        }

        /* ---- Speed: 0 to 200 ---- */
        if ((input->speed < SPEED_MIN) || (input->speed > SPEED_MAX)) {
            if (!g_suppress_io) {
                sys_printf("[INPUT] Invalid speed %d -> using last valid %d\n",
                       input->speed, status->last_valid_speed);
            }
            input->speed = status->last_valid_speed;
        } else {
            status->last_valid_speed = input->speed;
        }

        /* ---- Temperature: -40 to 150 ---- */
        if ((input->temperature < TEMP_MIN) || (input->temperature > TEMP_MAX)) {
            if (!g_suppress_io) {
                sys_printf("[INPUT] Invalid temperature %d -> using last valid %d\n",
                       input->temperature, status->last_valid_temperature);
            }
            input->temperature = status->last_valid_temperature;
        } else {
            status->last_valid_temperature = input->temperature;
        }

        /* ---- Gear: 0 to 5 ---- */
        if ((input->gear < GEAR_MIN) || (input->gear > GEAR_MAX)) {
            set_fault(faults, FAULT_INVALID_GEAR);   /* raise fault BEFORE correcting */
            if (!g_suppress_io) {
                sys_printf("[INPUT] Invalid gear %d -> using last valid %d\n",
                       input->gear, status->last_valid_gear);
            }
            input->gear = status->last_valid_gear;
        } else {
            status->last_valid_gear = input->gear;
        }

        /* ---- Requested Mode: valid enum only ---- */
        if (input->requested_mode == MODE_UNKNOWN) {
             if (!g_suppress_io) {
                sys_printf("[INPUT] Invalid mode -> using last valid %d\n",
                       status->last_valid_mode);
            }
            input->requested_mode = status->last_valid_mode;
        } else {
            status->last_valid_mode = input->requested_mode;
        }
    }
}
