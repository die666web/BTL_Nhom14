/* Nút (PB8, xem BSP_BUTTON_GPIO_BASE/BSP_BUTTON_PIN) — Vũ Văn Võ
 * Pull-up nội, nhấn nối GND (BSP_BUTTON_ACTIVE). Gọi button_poll_pressed() mỗi 20 ms.
 * Chống dội: mức đọc phải ổn định BUTTON_STABLE_SAMPLES lần liên tiếp (~60 ms) mới được nhận.
 * Trả true đúng một lần ở cạnh nhả -> nhấn; giữ nút không lặp. Không delay bận.
 */
#include "button.h"
#include "board_config.h"
#include "hal_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

#define BUTTON_STABLE_SAMPLES 3U
#define BUTTON_GPIO ((GPIO_TypeDef *)BSP_BUTTON_GPIO_BASE)

static bool stable_pressed;
static bool last_raw;
static uint8_t stable_count;

static bool read_raw_pressed(void)
{
    return GPIO_ReadPin(BUTTON_GPIO, BSP_BUTTON_PIN) == BSP_BUTTON_ACTIVE;
}

void button_init(void)
{
    taskENTER_CRITICAL();
    GPIO_ConfigPin(BUTTON_GPIO, BSP_BUTTON_PIN, GPIO_MODE_INPUT_PULLUP, GPIO_SPEED_INPUT);
    taskEXIT_CRITICAL();
    /* Nút đang giữ lúc khởi động không sinh sự kiện: phải nhả rồi nhấn lại. */
    last_raw = read_raw_pressed();
    stable_pressed = last_raw;
    stable_count = BUTTON_STABLE_SAMPLES;
}

bool button_poll_pressed(void)
{
    bool raw = read_raw_pressed();

    if (raw != last_raw) {
        last_raw = raw;
        stable_count = 1U;
        return false;
    }
    if (stable_count < BUTTON_STABLE_SAMPLES) {
        stable_count++;
        if (stable_count == BUTTON_STABLE_SAMPLES && raw != stable_pressed) {
            stable_pressed = raw;
            return raw; /* chỉ báo cạnh nhấn */
        }
    }
    return false;
}
