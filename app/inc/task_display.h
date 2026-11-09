#ifndef TASK_DISPLAY_H
#define TASK_DISPLAY_H
#include "FreeRTOS.h"
#include "task.h"
/* API đã thống nhất. Người phụ trách: Vũ Chí Công.
 * Chu kỳ 400ms: xQueuePeek hai mailbox, hiển thị U(V), I(mA), P(W), ON/OFF, version và lỗi. Không đọc INA219 trực tiếp.
 * Thay đổi API: trao đổi leader trước khi sửa. */
BaseType_t task_display_create(void);
#endif
