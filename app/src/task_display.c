/* CHƯA TRIỂN KHAI — Vũ Chí Công
 * File dành riêng cho phần việc này, không phải module đã chạy.
 * Đọc docs/PHAN_CONG.md và header cùng tên trước khi viết.
 * Chu kỳ 400ms: xQueuePeek hai mailbox, hiển thị U(V), I(mA), P(W), ON/OFF, version và lỗi. Không đọc INA219 trực tiếp.
 * Điền các hàm khai báo trong header; helper nội bộ để static.
 * Không sửa main.c, BSP, FreeRTOSConfig.h hoặc API chung trong nhánh này.
 * Không dùng delay_ms/HAL_Delay trong task, không chờ vô hạn.
 * Khi xong: test riêng trên kit, ghi kết quả vào tests/manual/, mở PR -> develop.
 */



/*
 * File: task_display.c
 * Author: Vu Chi Cong
 *
 * Description:
 * FreeRTOS task for OLED SSD1306 display.
 *
 * Features:
 * - Read measurement data from FreeRTOS mailbox.
 * - Read relay and OTA status from control mailbox.
 * - Detect invalid or outdated measurement data.
 * - Display voltage, current, power and status.
 * - Update display every 400 ms.
 * - Retry OLED initialization after I2C errors.
 *
 * Notes:
 * - Do not access INA219 directly.
 * - Do not control relay directly.
 * - Do not use blocking HAL_Delay().
 * - Do not modify shared application APIs.
 */

#include "task_display.h"
#include "app_cfg.h"
#include "app_ipc.h"
#include "oled.h"

/* Measurement data timeout: 1000 ms */
#define DISPLAY_DATA_TIMEOUT_MS 1000U

/* =====================================================
 * PRIVATE DISPLAY TASK
 * ===================================================== */

static void prvDisplayTask(void *pvParameters)
{
    (void)pvParameters;

    /* Local copies of mailbox data */
    app_measurement_t measurement = {0};
    app_control_state_t control = {0};

    /* Display update period */
    const TickType_t period =
        pdMS_TO_TICKS(APP_DISPLAY_PERIOD_MS);

    /* Maximum valid measurement age */
    const TickType_t dataTimeout =
        pdMS_TO_TICKS(DISPLAY_DATA_TIMEOUT_MS);

    TickType_t lastWakeTime =
        xTaskGetTickCount();

    BaseType_t oledReady = pdFALSE;

    for (;;)
    {
        /* =========================================
         * 1. INITIALIZE / RECONNECT OLED
         * ========================================= */

        if (oledReady == pdFALSE)
        {
            if (oled_init() == HAL_OK)
            {
                oledReady = pdTRUE;
            }
        }

        /* =========================================
         * 2. READ MEASUREMENT MAILBOX
         * ========================================= */

        /* Invalid by default if no data is received */
        measurement.valid = false;

        if (xQueuePeek(
                g_measurement_queue,
                &measurement,
                0U) == pdPASS)
        {
            if (measurement.valid)
            {
                TickType_t now =
                    xTaskGetTickCount();

                TickType_t age =
                    now -
                    (TickType_t)measurement.sample_tick;

                /* Check measurement timeout */
                if (age > dataTimeout)
                {
                    measurement.valid = false;
                }
            }
        }

        /* =========================================
         * 3. READ CONTROL MAILBOX
         * ========================================= */

        if (xQueuePeek(
                g_control_queue,
                &control,
                0U) != pdPASS)
        {
            /* Safe display defaults */
            control.relay_on = false;
            control.ota_locked = false;
        }

        /* =========================================
         * 4. UPDATE OLED DISPLAY
         * ========================================= */

        if (oledReady == pdTRUE)
        {
            if (oled_show(
                    &measurement,
                    &control,
                    APP_VERSION) != HAL_OK)
            {
                /* Retry initialization on next cycle */
                oledReady = pdFALSE;
            }
        }

        /* =========================================
         * 5. MAINTAIN 400 ms TASK PERIOD
         * ========================================= */

        (void)xTaskDelayUntil(
            &lastWakeTime,
            period
        );
    }
}

/* =====================================================
 * PUBLIC API: CREATE DISPLAY TASK
 * ===================================================== */

BaseType_t task_display_create(void)
{
    return xTaskCreate(
        prvDisplayTask,
        "Display",
        APP_DISPLAY_STACK,
        NULL,
        APP_DISPLAY_PRIO,
        NULL
    );
}
