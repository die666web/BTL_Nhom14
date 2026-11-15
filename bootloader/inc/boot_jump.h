#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H
/* Tắt SysTick + mọi IRQ, xoá pending, VTOR = OTA_APP_BASE, MSP = word đầu, nhảy Reset_Handler app.
 * Chỉ gọi sau port_deinit() và khi app đã được kiểm tra. Không trở về. */
void boot_jump_to_application(void) __attribute__((noreturn));
#endif
