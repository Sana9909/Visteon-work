#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* ─── Enumerations ─────────────────────────────────────────── */

typedef enum {
    MODE_OFF = 0,
    MODE_ACC,
    MODE_IGNITION_ON,
    MODE_FAULT
} VehicleMode;

typedef enum {
    STATE_NORMAL = 0,
    STATE_DEGRADED,
    STATE_SAFE
} SystemState;

typedef enum {
    FAULT_OVERSPEED    = 0,
    FAULT_OVERTEMP     = 1,
    FAULT_GEAR         = 2,
    FAULT_MODE         = 3,
    FAULT_REDLINE      = 4,  /* NEW: Engine RPM too high */
    FAULT_STALL        = 5,  /* NEW: Engine RPM too low */
    FAULT_COUNT        = 6   /* total number of fault types */
} FaultType;

/* ─── Structs ──────────────────────────────────────────────── */

typedef struct {
    int          speed;           /* km/h   : 0–200   */
    int          temperature;     /* °C     : -40–150 */
    int          gear;            /* pos    : 0–5     */
    VehicleMode  requested_mode;
    uint8_t      input_valid;     /* 1 = all inputs validated this cycle */
} VehicleInput;

typedef struct {
    VehicleMode  current_mode;
    VehicleMode  previous_mode;
    SystemState  system_state;
    SystemState  previous_state;
    uint32_t     cycle_count;
} VehicleStatus;

typedef struct {
    uint8_t  fault_flags;                  /* bitmask – bit N = FaultType N */
    int      fault_counters[FAULT_COUNT];  /* cumulative count per fault    */
    int      persistent_cycles[FAULT_COUNT]; /* consecutive active cycles   */
    uint8_t  persistent_flags;            /* faults active ≥ 3 cycles      */
} FaultStatus;

/* ─── Macros ───────────────────────────────────────────────── */


#define RPM_IDLE       800
#define RPM_REDLINE    6000
#define RPM_STALL      600

#define SPEED_MIN           0
#define SPEED_MAX           200
#define TEMP_MIN            (-40)
#define TEMP_MAX            150
#define GEAR_MIN            0
#define GEAR_MAX            5

#define OVERSPEED_THRESHOLD 120
#define TEMP_CRITICAL       110
#define TEMP_HIGH           95

#define PERSISTENT_THRESHOLD 3   /* cycles before fault is "persistent" */

#define SET_BIT(reg, bit)   ((reg) |=  (uint8_t)(1u << (bit)))
#define CLEAR_BIT(reg, bit) ((reg) &= ~(uint8_t)(1u << (bit)))
#define TEST_BIT(reg, bit)  (((reg) >>  (bit)) & 1u)

#endif /* TYPES_H */