/**
 * @file types.h
 * @brief Common enums, structs, typedefs, and macros shared across all modules.
 *
 * All shared data types are centralized here to maintain a single source of truth.
 * No logic or function implementations belong in this file.
 */

#ifndef TYPES_H_
#define TYPES_H_

#include <stdint.h>

#define INPUT_SENTINEL  (-32768)

/* ======================== Enumerations ======================== */

/**
 * @brief Vehicle operating modes.
 *
 * Transition rules:
 *   OFF  -> ACC            (allowed)
 *   ACC  -> IGNITION_ON    (allowed)
 *   ACC  -> OFF            (allowed)
 *   IGNITION_ON -> ACC     (allowed)
 *   IGNITION_ON -> OFF     (allowed)
 *   OFF  -> IGNITION_ON    (ILLEGAL - must go through ACC)
 *   FAULT -> any           (LOCKED - requires system reset)
 */
typedef enum {
    MODE_OFF          = 0,
    MODE_ACC          = 1,
    MODE_IGNITION_ON  = 2,
    MODE_FAULT        = 3,
    MODE_UNKNOWN      = 4   /* Added for MISRA compliance */
} Mode;

#define MODE_MIN (MODE_OFF)
#define MODE_MAX (MODE_FAULT)

/**
 * @brief System health states with escalation path.
 *   NORMAL -> DEGRADED -> SAFE
 *   SAFE is latched and requires MODE_OFF reset to recover.
 */
typedef enum {
    STATE_NORMAL   = 0,
    STATE_DEGRADED = 1,
    STATE_SAFE     = 2,
    STATE_UNKNOWN  = 3    /* Added for MISRA compliance */
} SystemState;

#define STATE_MIN (STATE_NORMAL)
#define STATE_MAX (STATE_SAFE)

/* ======================== Fault Bit Masks ======================== */

typedef uint32_t FaultFlags;

#define FAULT_NONE               (0U)
#define FAULT_OVERSPEED          (1U << 0U)   /* bit 0 */
#define FAULT_OVERTEMP_CRITICAL  (1U << 1U)   /* bit 1 */
#define FAULT_OVERTEMP_HIGH      (1U << 2U)   /* bit 2 */
#define FAULT_INVALID_GEAR       (1U << 3U)   /* bit 3 */
#define FAULT_ILLEGAL_MODE       (1U << 4U)   /* bit 4 */

#define NUM_FAULT_TYPES          5U

/* ======================== Input Validation Ranges ======================== */

#define SPEED_MIN       (0)
#define SPEED_MAX       (200)
#define TEMP_MIN        (-40)
#define TEMP_MAX        (150)
#define GEAR_MIN        (0)
#define GEAR_MAX        (5)

/* ======================== Control Thresholds ======================== */

#define OVERSPEED_THRESHOLD         (120)
#define HIGH_TEMP_THRESHOLD         (95)
#define CRITICAL_TEMP_THRESHOLD     (110)

/* ======================== Structures ======================== */

/**
 * @brief Raw vehicle inputs for one ECU cycle.
 */
typedef struct {
    int16_t speed;
    int16_t temperature;
    int8_t  gear;
    Mode    requested_mode;
} VehicleInput;

/**
 * @brief Vehicle status maintained across cycles.
 */
typedef struct {
    Mode        current_mode;
    Mode        previous_mode;
    SystemState system_state;
    int16_t     last_valid_speed;
    int16_t     last_valid_temperature;
    int8_t      last_valid_gear;
    Mode        last_valid_mode;
} VehicleStatus;

/**
 * @brief Fault tracking with bitwise flags and per-fault counters.
 *
 * active_faults:  set/cleared each cycle (per-cycle snapshot)
 * *_counter:      cumulative across cycles (persistence tracking)
 */
typedef struct {
    FaultFlags active_faults;
    uint16_t   overspeed_counter;
    uint16_t   overtemp_critical_counter;
    uint16_t   overtemp_high_counter;
    uint16_t   invalid_gear_counter;
    uint16_t   illegal_mode_counter;
} FaultStatus;

#endif /* TYPES_H_ */
