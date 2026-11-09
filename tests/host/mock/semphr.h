#ifndef MOCK_SEMPHR_H
#define MOCK_SEMPHR_H
#include "FreeRTOS.h"
typedef struct MockMutex *SemaphoreHandle_t;
SemaphoreHandle_t xSemaphoreCreateMutex(void);
BaseType_t xSemaphoreTake(SemaphoreHandle_t m, TickType_t timeout);
BaseType_t xSemaphoreGive(SemaphoreHandle_t m);
void vSemaphoreDelete(SemaphoreHandle_t m);
#endif
