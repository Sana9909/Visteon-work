#include <stdio.h>
#include "input.h"
#include "log.h"

/* Shadow copy of last valid input values */
static VehicleInput s_last_valid = {
    .speed          = 0,
    .temperature    = 20,
    .gear           = 0,
    .requested_mode = MODE_OFF,
    .input_valid    = 1
};

void init_last_valid(void)
{
    s_last_valid.speed          = 0;
    s_last_valid.temperature    = 20;
    s_last_valid.gear           = 0;
    s_last_valid.requested_mode = MODE_OFF;
    s_last_valid.input_valid    = 1;
}

void read_inputs(VehicleInput *input)
{
    int mode_raw = 0;

    printf("\n--- Enter Inputs ---\n");
    printf("Speed (0-200)        : ");
    scanf("%d", &input->speed);

    printf("Temperature (-40-150): ");
    scanf("%d", &input->temperature);

    printf("Gear (0-5)           : ");
    scanf("%d", &input->gear);

    printf("Mode (0=OFF 1=ACC 2=IGNITION_ON 3=FAULT): ");
    scanf("%d", &mode_raw);
    input->requested_mode = (VehicleMode)mode_raw;

    input->input_valid = 1; /* will be cleared by validate if needed */
}

void validate_inputs(VehicleInput *input, VehicleStatus *status)
{
    (void)status; /* reserved for future use */

    /* --- Speed --- */
    if (input->speed < SPEED_MIN || input->speed > SPEED_MAX) {
        log_warning("Invalid speed %d – using last valid %d",
                    input->speed, s_last_valid.speed);
        input->speed   = s_last_valid.speed;
        input->input_valid = 0;
    } else {
        s_last_valid.speed = input->speed;
    }

    /* --- Temperature --- */
    if (input->temperature < TEMP_MIN || input->temperature > TEMP_MAX) {
        log_warning("Invalid temperature %d – using last valid %d",
                    input->temperature, s_last_valid.temperature);
        input->temperature = s_last_valid.temperature;
        input->input_valid = 0;
    } else {
        s_last_valid.temperature = input->temperature;
    }

    /* --- Gear --- */
    if (input->gear < GEAR_MIN || input->gear > GEAR_MAX) {
        log_warning("Invalid gear %d – using last valid %d",
                    input->gear, s_last_valid.gear);
        input->gear    = s_last_valid.gear;
        input->input_valid = 0;
    } else {
        s_last_valid.gear = input->gear;
    }

    /* --- Mode enum range --- */
    if ((int)input->requested_mode < 0 ||
        (int)input->requested_mode > (int)MODE_FAULT) {
        log_warning("Invalid mode value %d – using last valid %d",
                    (int)input->requested_mode,
                    (int)s_last_valid.requested_mode);
        input->requested_mode = s_last_valid.requested_mode;
        input->input_valid    = 0;
    } else {
        s_last_valid.requested_mode = input->requested_mode;
    }
}