#ifndef BOOT_FLASH_H
#define BOOT_FLASH_H
/* Flash nội STM32F103: page 1 KiB, ghi half-word tại địa chỉ chẵn.
 * Chỉ cho phép xoá/ghi trong [OTA_APP_BASE, OTA_FLASH_END) — không bao giờ đụng vùng bootloader. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool boot_flash_erase_page(uint32_t page_addr);
/* Ghi len byte từ addr (chẵn). len lẻ: byte cuối được đệm 0xFF. Đọc lại kiểm tra từng half-word. */
bool boot_flash_program(uint32_t addr, const uint8_t *data, size_t len);
bool boot_flash_program_u16(uint32_t addr, uint16_t value);
/* Con trỏ đọc Flash (target: chính địa chỉ; PC: mảng giả lập). */
const uint8_t *boot_flash_ptr(uint32_t addr);

#endif
