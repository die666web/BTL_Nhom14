/* Relay (PA0, xem BSP_RELAY_GPIO_BASE/BSP_RELAY_PIN) — Vũ Văn Võ
 * Mức kích lấy từ BSP_RELAY_ACTIVE (board_config.h). Chỉ TaskControl gọi set/get;
 * BSP chỉ ép OFF lúc khởi động/panic/OTA. Không PWM.
 */
#include "relay.h"
#include "board_config.h"
#include "hal_gpio.h"
#include "FreeRTOS.h"
#include "task.h"

#define RELAY_GPIO ((GPIO_TypeDef *)BSP_RELAY_GPIO_BASE)

static bool relay_on;

static void write_pin(bool on)
{
    /* BSRR/BRR: ghi nguyên tử, không cần khóa. */
    GPIO_WritePin(RELAY_GPIO, BSP_RELAY_PIN, on ? BSP_RELAY_ACTIVE : !BSP_RELAY_ACTIVE);
}

void relay_init(void)
{
    /* Nạp mức OFF trước rồi mới (cấu hình lại) output, tránh xung ON ngắn.
     * Khóa ngắn vì GPIO_ConfigPin là read-modify-write CRL/CRH dùng chung port. */
    relay_on = false;
    write_pin(false);
    taskENTER_CRITICAL();
    GPIO_ConfigPin(RELAY_GPIO, BSP_RELAY_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_2MHZ);
    taskEXIT_CRITICAL();
}

void relay_set(bool on)
{
    relay_on = on;
    write_pin(on);
}

bool relay_get(void)
{
    return relay_on;
}
