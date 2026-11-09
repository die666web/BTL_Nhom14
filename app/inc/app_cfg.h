#ifndef APP_CFG_H
#define APP_CFG_H
#include "FreeRTOS.h"
#include "task.h"

/* Leader enables a task only after its implementation is merged and tested.
 * No dummy task returns success. Enabling an empty module causes a link error. */
#define APP_ENABLE_MEASURE  0
#define APP_ENABLE_CONTROL  0
#define APP_ENABLE_DISPLAY  0
#define APP_ENABLE_COMM     0
#define APP_VERSION         "0.1.0-scaffold"
#define APP_MEASURE_PERIOD_MS 200U
#define APP_CONTROL_PERIOD_MS  20U
#define APP_DISPLAY_PERIOD_MS 400U
#define APP_COMMAND_QUEUE_LEN   8U
#define APP_UART_RX_QUEUE_LEN 128U
/* Stack sizes in 32-bit words; initial budgets, measure high-water marks on kit. */
#define APP_MEASURE_STACK 256U
#define APP_CONTROL_STACK 192U
#define APP_DISPLAY_STACK 384U
#define APP_COMM_STACK    384U
#define APP_MEASURE_PRIO (tskIDLE_PRIORITY + 2U)
#define APP_CONTROL_PRIO (tskIDLE_PRIORITY + 3U)
#define APP_DISPLAY_PRIO (tskIDLE_PRIORITY + 1U)
#define APP_COMM_PRIO    (tskIDLE_PRIORITY + 3U)
#endif
