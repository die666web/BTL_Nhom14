#ifndef BSP_H
#define BSP_H
void bsp_system_init(void);
void bsp_relay_force_off(void);
void bsp_panic(void) __attribute__((noreturn));
/* OTA: relay OFF, ghi BKP_DR1 = OTA_BKP_FLAG_UPDATE, software reset -> bootloader chờ BEGIN.
 * Gọi từ task sau khi đã xong handshake OTA_REQUEST/OFF_ACK. */
void bsp_reboot_to_bootloader(void) __attribute__((noreturn));
#endif
