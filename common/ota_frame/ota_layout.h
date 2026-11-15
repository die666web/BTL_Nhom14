#ifndef OTA_LAYOUT_H
#define OTA_LAYOUT_H
/* Flash layout STM32F103C8 dùng chung cho bootloader, app và ESP32 (kiểm tra file .bin).
 * Đổi số ở đây thì đổi cả: STM32F103C8Tx_FLASH.ld, bootloader/STM32F103C8_BOOT.ld,
 * keil/STM32F103C8.sct, keil/Bootloader.sct, tools/sync_projects.py.
 *
 *   0x08000000 .. 0x08002000  Bootloader  8 KiB
 *   0x08002000 .. 0x0800FC00  App        55 KiB (56320 B)
 *   0x0800FC00 .. 0x08010000  Metadata    1 page
 */
#define OTA_FLASH_BASE      0x08000000u
#define OTA_FLASH_END       0x08010000u
#define OTA_PAGE_SIZE       1024u

#define OTA_APP_BASE        0x08002000u
#define OTA_APP_LIMIT       0x0800FC00u
#define OTA_APP_MAX_SIZE    (OTA_APP_LIMIT - OTA_APP_BASE)

#define OTA_META_BASE       0x0800FC00u   /* +0 size u32, +4 image_crc32 u32, +8 valid u16 */
#define OTA_META_VALID      0xA55Au

#define OTA_SRAM_BASE       0x20000000u
#define OTA_SRAM_END        0x20005000u   /* ngay sau SRAM 20 KiB */

/* App ghi giá trị này vào BKP_DR1 rồi reset để bootloader ở lại chờ BEGIN. */
#define OTA_BKP_FLAG_UPDATE 0xB007u

#endif
