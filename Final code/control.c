#include "control.h"
#include "fault.h"
#include "log.h"

/* Gear multipliers to simulate RPM = Speed * Gear_Ratio */
static const int gear_multipliers[6] = {
    0,   /* Gear 0: Neutral */
    120, /* Gear 1 */
    70,  /* Gear 2 */
    50,  /* Gear 3 */
    35,  /* Gear 4 */
    25   /* Gear 5 */
};

void run_control_checks(const VehicleInput *input,
                        VehicleStatus      *status,
                        FaultStatus        *faults)
{
    /* ── Priority 1: Critical Overheat ── */
    /* Heat checks run in ALL modes (Heat Soak simulation) */
    if (input->temperature > TEMP_CRITICAL) {
        log_fault("CRITICAL OVERHEAT: temperature %d°C (threshold %d°C)",
                  input->temperature, TEMP_CRITICAL);
        set_fault(faults, FAULT_OVERTEMP);
    } else {
        clear_fault(faults, FAULT_OVERTEMP);
    }

    /* ── Sensor & Motion Checks (Awake Modes Only) ── */
    if (status->current_mode == MODE_ACC || status->current_mode == MODE_IGNITION_ON) {
        
        /* ── Priority 2: Invalid Gear ── */
        if (input->gear < GEAR_MIN || input->gear > GEAR_MAX) {
            log_fault("INVALID GEAR: %d (valid range %d-%d)",
                      input->gear, GEAR_MIN, GEAR_MAX);
            set_fault(faults, FAULT_GEAR);
        }
        else {
            clear_fault(faults, FAULT_GEAR);

            /* ── Priority 3: Overspeed ── */
            if (input->speed > OVERSPEED_THRESHOLD) {
                log_fault("OVERSPEED: %d km/h (threshold %d km/h)",
                          input->speed, OVERSPEED_THRESHOLD);
                set_fault(faults, FAULT_OVERSPEED);
            } else {
                clear_fault(faults, FAULT_OVERSPEED);
            }

            /* ── Priority 4: Engine RPM Checks (Ignition ON Only) ── */
            if (status->current_mode == MODE_IGNITION_ON) {
                int calculated_rpm = RPM_IDLE; 
                if (input->gear > 0 && input->gear <= GEAR_MAX) {
                    calculated_rpm = input->speed * gear_multipliers[input->gear];
                }

                if (calculated_rpm > RPM_REDLINE) {
                    log_fault("ENGINE REDLINE! RPM %d (Limit: %d)", calculated_rpm, RPM_REDLINE);
                    set_fault(faults, FAULT_REDLINE);
                } else {
                    clear_fault(faults, FAULT_REDLINE);
                }

                if (calculated_rpm < RPM_STALL && input->speed > 0 && input->gear > 0) {
                    log_fault("ENGINE STALL! RPM %d is too low to maintain speed.", calculated_rpm);
                    set_fault(faults, FAULT_STALL);
                } else {
                    clear_fault(faults, FAULT_STALL);
                }
            } else {
                /* If ACC mode, engine is not running */
                clear_fault(faults, FAULT_REDLINE);
                clear_fault(faults, FAULT_STALL);
            }
        }
    } 
    /* If MODE_OFF or MODE_FAULT, clear motion/sensor faults automatically */
    else {
        clear_fault(faults, FAULT_GEAR);
        clear_fault(faults, FAULT_OVERSPEED);
        clear_fault(faults, FAULT_REDLINE);
        clear_fault(faults, FAULT_STALL);
    }

    /* ── Priority 5: High Temperature Warning (Runs in all modes) ── */
    if (input->temperature > TEMP_HIGH && input->temperature <= TEMP_CRITICAL) {
        log_warning("HIGH TEMPERATURE: %d°C (warning threshold %d°C)",
                    input->temperature, TEMP_HIGH);
    }
}