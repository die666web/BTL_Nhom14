#ifndef BOOT_PROTOCOL_H
#define BOOT_PROTOCOL_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * Nhận gói UART có length/sequence/CRC, timeout và ACK/NACK. Dùng chung đặc tả docs/OTA_CONTRACT.md với ESP32. Không phụ thuộc FreeRTOS app.
 * Thay đổi API: trao đổi leader trước khi sửa. */
void boot_protocol_run(void);
#endif
