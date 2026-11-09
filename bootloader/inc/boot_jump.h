#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* API đã thống nhất. Người phụ trách: Lê Trọng Mạnh.
 * Dừng IRQ/SysTick, đặt VTOR/MSP, nhảy reset handler đúng thứ tự. Đồng bộ clock handoff với BSP. Chỉ gọi sau validate.
 * Thay đổi API: trao đổi leader trước khi sửa. */
void boot_jump_to_application(void);
#endif
