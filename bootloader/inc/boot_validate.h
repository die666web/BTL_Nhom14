#ifndef BOOT_VALIDATE_H
#define BOOT_VALIDATE_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * Kiểm tra header/CRC/kích thước/MSP/reset vector; invalid thì ở lại nhận lại firmware. Không xem CRC là xác thực chữ ký.
 * Thay đổi API: trao đổi leader trước khi sửa. */
bool boot_application_valid(void);
#endif
