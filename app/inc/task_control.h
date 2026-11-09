#ifndef TASK_CONTROL_H
#define TASK_CONTROL_H
#include "FreeRTOS.h"
#include "task.h"
/* API đã thống nhất. Người phụ trách: Vũ Văn Võ.
 * Chu kỳ 20ms: đọc nút/chống dội, nhận g_command_queue, điều khiển relay. Ưu tiên OTA_REQUEST; khóa ON, tắt relay rồi set OFF_ACK. Publish g_control_queue.
 * Thay đổi API: trao đổi leader trước khi sửa. */
BaseType_t task_control_create(void);
#endif
