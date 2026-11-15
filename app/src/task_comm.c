/* TaskComm — Lê Trọng Mạnh.
 * Chủ duy nhất của UART RX. Dò frame OTA v1 (common/ota_frame) trên USART1.
 * Hiện xử lý ENTER_UPDATE (docs/OTA_CONTRACT.md mục 5.1):
 *   1. Clear OFF_ACK cũ, set OTA_REQUEST.
 *   2. Chờ Control tắt relay và set OFF_ACK (có hạn). Control chưa bật/không trả lời: BSP vẫn ép relay OFF.
 *   3. Ghi cờ BKP_DR1 rồi software reset -> bootloader gửi READY.
 * Các frame khác bị bỏ qua trong app. Không log text trên UART này.
 * TODO (nếu cần): lệnh sản phẩm (ON/OFF, đọc version) nên dùng cùng khung frame, TYPE riêng. */
#include "task_comm.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "bsp.h"
#include "hal_uart.h"
#include "ota_frame.h"

#define COMM_READ_TIMEOUT_MS   50U
#define COMM_OFF_ACK_TIMEOUT_MS 500U   /* ESP32 chờ READY 1000 ms: còn đủ thời gian reset */

static uint32_t now_ms(void) { return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS); }

static void enter_bootloader(void)
{
    (void)xEventGroupClearBits(g_system_events, APP_EVENT_RELAY_OFF_ACK);
    (void)xEventGroupSetBits(g_system_events, APP_EVENT_OTA_REQUEST);
#if APP_ENABLE_CONTROL
    (void)xEventGroupWaitBits(g_system_events, APP_EVENT_RELAY_OFF_ACK,
                              pdFALSE, pdTRUE, pdMS_TO_TICKS(COMM_OFF_ACK_TIMEOUT_MS));
#endif
    bsp_reboot_to_bootloader();      /* relay OFF + BKP_DR1 + reset, không trở về */
}

static void comm_task(void *arg)
{
    (void)arg;
    ota_parser_t rx;
    ota_parser_reset(&rx);
    uint32_t errors = hal_uart_rx_error_count();

    for (;;) {
        uint8_t byte;
        if (hal_uart_read(&byte, pdMS_TO_TICKS(COMM_READ_TIMEOUT_MS)) != HAL_OK) continue;

        /* Driver đã bỏ byte lỗi (ORE/FE/NE/PE): frame đang nhận không còn tin được. */
        uint32_t e = hal_uart_rx_error_count();
        if (e != errors) {
            errors = e;
            ota_parser_on_uart_error(&rx);
        }

        if (!ota_parser_feed(&rx, byte, now_ms())) continue;
        if (rx.body[0] == OTA_T_ENTER_UPDATE && rx.len == 1U)
            enter_bootloader();
    }
}

BaseType_t task_comm_create(void)
{
    return xTaskCreate(comm_task, "comm", APP_COMM_STACK, NULL, APP_COMM_PRIO, NULL);
}
