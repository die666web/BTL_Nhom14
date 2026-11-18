#ifndef TASK_COMM_H
#define TASK_COMM_H
#include "FreeRTOS.h"
#include "task.h"
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * Nhận byte UART có timeout, parse gói OTA (chỉ ENTER_UPDATE). OTA: clear ACK cũ, set REQUEST, chờ OFF_ACK hữu hạn trước reset. Không tự điều khiển relay.
 * Thay đổi API: trao đổi leader trước khi sửa. */
BaseType_t task_comm_create(void);
#endif
