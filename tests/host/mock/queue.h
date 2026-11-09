#ifndef MOCK_QUEUE_H
#define MOCK_QUEUE_H
#include "FreeRTOS.h"
typedef struct MockQueue *QueueHandle_t;
QueueHandle_t xQueueCreate(UBaseType_t count, UBaseType_t size);
BaseType_t xQueueSend(QueueHandle_t queue, const void *data, TickType_t timeout);
BaseType_t xQueueReceive(QueueHandle_t queue, void *data, TickType_t timeout);
BaseType_t xQueueSendFromISR(QueueHandle_t queue, const void *data, BaseType_t *wake);
BaseType_t xQueueReceiveFromISR(QueueHandle_t queue, void *data, BaseType_t *wake);
void vQueueDelete(QueueHandle_t queue);
#endif
