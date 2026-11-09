#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * Kiểm tra biên ứng dụng, alignment, erase/program/verify; không ghi vùng bootloader. Chốt memory map trước khi triển khai.
 * Thay đổi API: trao đổi leader trước khi sửa. */
bool boot_flash_write(uint32_t address, const uint8_t *data, size_t length);
#endif
