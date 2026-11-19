#ifndef TASK_COMM_H
#define TASK_COMM_H
#include "FreeRTOS.h"
#include "task.h"
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * Đọc UART từ ESP32, chờ chuỗi lệnh "#ENTER_BOOTLOADER#". Khi nhận được: xoá ACK cũ, đặt OTA_REQUEST,
 * chờ OFF_ACK có hạn, rồi reset vào bootloader ROM của ST. Không tự điều khiển relay.
 * Thay đổi API: trao đổi leader trước khi sửa. */
BaseType_t task_comm_create(void);
#endif
