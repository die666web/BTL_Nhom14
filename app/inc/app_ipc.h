#ifndef APP_IPC_H
#define APP_IPC_H
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "app_types.h"

/* Leader owns creation. Init once before tasks.
 * Mailboxes length=1: producer xQueueOverwrite, consumer xQueuePeek.
 * Do not Receive a display mailbox: other readers must keep seeing latest state. */
extern QueueHandle_t g_measurement_queue; /* app_measurement_t; Measure -> Display */
extern QueueHandle_t g_control_queue;     /* app_control_state_t; Control -> Display */
extern QueueHandle_t g_command_queue;     /* app_command_t; Comm -> Control */
extern SemaphoreHandle_t g_i2c_mutex;     /* acquired INSIDE hal_i2c_* per transaction */
extern EventGroupHandle_t g_system_events;
#define APP_EVENT_OTA_REQUEST (1U << 0)
#define APP_EVENT_RELAY_OFF_ACK (1U << 1)
/* Comm sets OTA_REQUEST; Control locks ON, forces OFF, publishes OFF_ACK.
 * Comm MUST wait bounded time for OFF_ACK before reset/jump. Clear stale ACK first.
 * Events used from task context only. Do not wait for ACK while holding a mutex. */
BaseType_t app_ipc_init(void);
#endif
