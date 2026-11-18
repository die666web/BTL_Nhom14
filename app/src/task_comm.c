/* TaskComm — Lê Trọng Mạnh.
 * Chủ duy nhất của UART RX (USART1, nối ESP32). Chỉ làm một việc: chờ ESP32 gửi
 * chuỗi lệnh "vào chế độ cập nhật", rồi chuẩn bị và reset vào bootloader ROM của ST:
 *   1. Xoá OFF_ACK cũ, đặt OTA_REQUEST.
 *   2. Chờ TaskControl tắt relay và đặt OFF_ACK (có hạn). TaskControl không trả lời: BSP vẫn ép relay OFF.
 *   3. Ghi cờ vào BKP_DR1 rồi reset -> bsp_system_init() nhảy vào bootloader ROM.
 * Sau đó ESP32 nói chuyện trực tiếp với bootloader ROM (giao thức AN3155) để nạp firmware.
 * Không có điều khiển relay từ xa: relay chỉ bật/tắt bằng nút (TaskControl). */
#include "task_comm.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "bsp.h"
#include "hal_uart.h"

/* Chuỗi lệnh ESP32 gửi xuống. PHẢI khớp ENTER_COMMAND trong esp32/src/uart_bridge.cpp.
 * Chuỗi dài, nhiễu ngẫu nhiên trên dây gần như không thể trùng. */
static const char ENTER_COMMAND[] = "#ENTER_BOOTLOADER#";
#define ENTER_COMMAND_LENGTH  (sizeof(ENTER_COMMAND) - 1U)

#define COMM_OFF_ACK_TIMEOUT_MS  500U


/* Bật cờ OTA, chờ relay tắt, rồi reset vào bootloader. Không trở về. */
static void enter_bootloader(void)
{
    (void)xEventGroupClearBits(g_system_events, APP_EVENT_RELAY_OFF_ACK);
    (void)xEventGroupSetBits(g_system_events, APP_EVENT_OTA_REQUEST);

#if APP_ENABLE_CONTROL
    (void)xEventGroupWaitBits(g_system_events, APP_EVENT_RELAY_OFF_ACK,
                              pdFALSE, pdTRUE, pdMS_TO_TICKS(COMM_OFF_ACK_TIMEOUT_MS));
#endif

    bsp_reboot_to_bootloader();      /* relay OFF + ghi cờ BKP_DR1 + reset */
}

/* So từng byte nhận được với ENTER_COMMAND.
 * matched = số ký tự đầu đã khớp. Trả 1 khi khớp đủ cả chuỗi. */
static uint8_t match_enter_command(uint8_t byte, uint32_t *matched)
{
    if (byte == (uint8_t)ENTER_COMMAND[*matched])
    {
        (*matched)++;
    }
    else if (byte == (uint8_t)ENTER_COMMAND[0])
    {
        *matched = 1U;               /* sai giữa chừng nhưng byte này lại là ký tự đầu */
    }
    else
    {
        *matched = 0U;
    }

    if (*matched == ENTER_COMMAND_LENGTH)
    {
        *matched = 0U;
        return 1U;
    }

    return 0U;
}

static void comm_task(void *arg)
{
    (void)arg;

    uint32_t matched = 0U;

    for (;;)
    {
        uint8_t byte;

        /* RX buffer rỗng: nhường CPU 1 ms (buffer 128 B ≈ 11 ms ở 115200, dư sức). */
        if (!ReceiveInterrupt_UART(&byte))
        {
            vTaskDelay(1);
            continue;
        }

        if (match_enter_command(byte, &matched))
        {
            enter_bootloader();
        }
    }
}

BaseType_t task_comm_create(void)
{
    return xTaskCreate(comm_task, "comm", APP_COMM_STACK, NULL, APP_COMM_PRIO, NULL);
}
