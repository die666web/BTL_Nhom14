/* Bootloader STM32F103C8 — điểm vào riêng, KHÔNG build chung với application. Lê Trọng Mạnh.
 * Nằm ở 0x08000000 (8 KiB). Luồng (docs/OTA_CONTRACT.md mục 5.1):
 *   1. Đọc và xoá cờ BKP_DR1.
 *   2. Không có cờ và app hợp lệ (marker, size, vector, CRC-32) -> chạy app ngay.
 *   3. Ngược lại: bật UART, gửi READY, chạy máy trạng thái tới khi nạp xong -> chạy app.
 * Không dùng FreeRTOS, không ngắt UART. Không log trên UART OTA. */
#include <stdbool.h>
#include "boot_flash.h"
#include "boot_jump.h"
#include "boot_port.h"
#include "boot_protocol.h"
#include "boot_validate.h"

int main(void)
{
    bool update_requested = port_take_update_flag();

    if (!update_requested && boot_application_valid())
        boot_jump_to_application();           /* chưa bật ngoại vi nào ngoài BKP (đã tắt lại) */

    port_init();
    boot_protocol_init();

    for (;;) {
        boot_protocol_poll();

        /* LED: chậm khi chờ BEGIN, nhanh khi đang nhận */
        uint32_t period = (boot_protocol_state() == BOOT_ST_RECEIVING) ? 100u : 500u;
        port_led(((port_ms() / period) & 1u) != 0u);

        if (boot_protocol_state() == BOOT_ST_RUN_APP) {
            if (boot_application_valid()) {
                port_deinit();
                boot_jump_to_application();
            }
            boot_protocol_init();             /* không thể xảy ra sau commit đúng; về WAIT_BEGIN cho an toàn */
        }
    }
}
