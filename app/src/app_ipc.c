#include "app_ipc.h"
#include "app_cfg.h"

QueueHandle_t g_measurement_queue;
QueueHandle_t g_control_queue;
QueueHandle_t g_command_queue;
SemaphoreHandle_t g_i2c_mutex;
EventGroupHandle_t g_system_events;

BaseType_t app_ipc_init(void)
{
    if (g_measurement_queue != NULL) { return pdFAIL; } /* init once */
    g_measurement_queue = xQueueCreate(1, sizeof(app_measurement_t));
    g_control_queue = xQueueCreate(1, sizeof(app_control_state_t));
    g_command_queue = xQueueCreate(APP_COMMAND_QUEUE_LEN, sizeof(app_command_t));
    g_i2c_mutex = xSemaphoreCreateMutex();
    g_system_events = xEventGroupCreate();
    if (!g_measurement_queue || !g_control_queue || !g_command_queue ||
        !g_i2c_mutex || !g_system_events) {
        if (g_measurement_queue) vQueueDelete(g_measurement_queue);
        if (g_control_queue) vQueueDelete(g_control_queue);
        if (g_command_queue) vQueueDelete(g_command_queue);
        if (g_i2c_mutex) vSemaphoreDelete(g_i2c_mutex);
        if (g_system_events) vEventGroupDelete(g_system_events);
        g_measurement_queue = g_control_queue = g_command_queue = NULL;
        g_i2c_mutex = NULL;
        g_system_events = NULL;
        return pdFAIL;
    }
    const app_measurement_t measurement = {0};
    const app_control_state_t control = {0};
    (void)xQueueOverwrite(g_measurement_queue, &measurement);
    (void)xQueueOverwrite(g_control_queue, &control);
    return pdPASS;
}
