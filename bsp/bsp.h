#ifndef BSP_H
#define BSP_H
void bsp_relay_force_off(void);
void bsp_panic(void) __attribute__((noreturn));
/* Relay OFF, VTOR, nút nhấn. Nếu có cờ BSP_BKP_FLAG_ENTER_BOOTLOADER thì nhảy vào bootloader ROM
 * của ST (không trở về). Gọi đầu tiên trong main(). */
void bsp_system_init(void);

/* OTA: relay OFF, ghi cờ vào BKP_DR1, software reset -> bsp_system_init() nhảy vào bootloader ROM.
 * Gọi từ task sau khi đã xong handshake OTA_REQUEST/OFF_ACK. */
void bsp_reboot_to_bootloader(void) __attribute__((noreturn));
#endif
