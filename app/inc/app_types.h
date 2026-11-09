#ifndef APP_TYPES_H
#define APP_TYPES_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float voltage_v;
    float current_a;  /* signed; convert to mA only for presentation */
    float power_w;
    uint32_t sample_tick;
    bool valid;       /* false on missing sensor; do not display stale data as valid */
} app_measurement_t;

typedef enum { APP_CMD_OFF = 0, APP_CMD_ON, APP_CMD_TOGGLE, APP_CMD_ENTER_OTA } app_command_t;
typedef struct { bool relay_on; bool ota_locked; } app_control_state_t;
#endif
