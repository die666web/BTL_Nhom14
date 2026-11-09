#include "FreeRTOS.h"
#include "task.h"
#include "bsp.h"

void vApplicationMallocFailedHook(void) { bsp_panic(); }
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task; (void)name;
    bsp_panic();
}
void app_assert_failed(const char *file, unsigned long line)
{
    (void)file; (void)line; /* inspect in debugger */
    bsp_panic();
}
