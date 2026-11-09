#ifndef MOCK_TASK_H
#define MOCK_TASK_H
#include "FreeRTOS.h"
#define taskSCHEDULER_RUNNING 2
#define tskIDLE_PRIORITY 0U
#define taskENTER_CRITICAL() ((void)0)
#define taskEXIT_CRITICAL() ((void)0)
TickType_t xTaskGetTickCount(void);
BaseType_t xTaskGetSchedulerState(void);
#endif
