#ifndef TASK_MEASURE_H
#define TASK_MEASURE_H
#include "FreeRTOS.h"
#include "task.h"
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * Chu kỳ 200ms: gọi INA219, lọc, tính P=U*I; publish app_measurement_t qua xQueueOverwrite. valid=false khi lỗi. Không truy cập OLED/relay.
 * Thay đổi API: trao đổi leader trước khi sửa. */
BaseType_t task_measure_create(void);
#endif
