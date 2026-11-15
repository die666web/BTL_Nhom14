#ifndef STM32_MEMORY_MAP_H
#define STM32_MEMORY_MAP_H
/* Địa chỉ STM32 mà ESP32 dùng để kiểm tra file .bin trước khi lưu.
 * Nguồn duy nhất: common/ota_frame/ota_layout.h (dùng chung với bootloader + app). */
#include "ota_layout.h"

#define STM32_APP_START     OTA_APP_BASE        /* 0x08002000 */
#define STM32_APP_END       OTA_APP_LIMIT       /* 0x0800FC00 (metadata ở sau) */
#define STM32_APP_MAX_SIZE  OTA_APP_MAX_SIZE    /* 56320 B */
#define STM32_SRAM_START    OTA_SRAM_BASE
#define STM32_SRAM_END      OTA_SRAM_END

#endif
