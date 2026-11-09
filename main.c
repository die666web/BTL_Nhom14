#include "bsp.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "hal_uart.h"
#include "hal_i2c.h"
#include "task_measure.h"
#include "task_control.h"
#include "task_display.h"
#include "task_comm.h"

/* LE TRONG MANH ONLY: integration entry point. Members edit their own task files. */
int main(void)
{
    bsp_system_init();
    if (app_ipc_init() != pdPASS) bsp_panic();
    if (Config_uart() != pdPASS) bsp_panic();
#if APP_ENABLE_MEASURE || APP_ENABLE_DISPLAY
    if (hal_i2c_init() != HAL_OK) bsp_panic();
#endif
#if APP_ENABLE_MEASURE
    if (task_measure_create() != pdPASS) bsp_panic();
#endif
#if APP_ENABLE_CONTROL
    if (task_control_create() != pdPASS) bsp_panic();
#endif
#if APP_ENABLE_DISPLAY
    if (task_display_create() != pdPASS) bsp_panic();
#endif
#if APP_ENABLE_COMM
    if (task_comm_create() != pdPASS) bsp_panic();
#endif
    /* Default: FreeRTOS idle/timer tasks only, relay OFF, no product feature. */
    vTaskStartScheduler();
    bsp_panic();
}
