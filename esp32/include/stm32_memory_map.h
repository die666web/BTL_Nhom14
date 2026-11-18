#ifndef STM32_MEMORY_MAP_H
#define STM32_MEMORY_MAP_H
/* Địa chỉ STM32F103C8 mà ESP32 dùng để kiểm tra file .bin và nạp.
 * App nằm ngay đầu Flash (không còn bootloader riêng), dùng cả 64 KiB.
 * Phải khớp STM32F103C8Tx_FLASH.ld và keil/STM32F103C8.sct. */

#define STM32_APP_START     0x08000000UL
#define STM32_APP_MAX_SIZE  (64UL * 1024UL)
#define STM32_SRAM_START    0x20000000UL
#define STM32_SRAM_END      0x20005000UL      /* ngay sau SRAM 20 KiB */

#endif
