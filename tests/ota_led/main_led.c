/* App thử OTA: chỉ bật LED PC13 (sáng liên tục) — Lê Trọng Mạnh.
 * Build: make ota-led  ->  build/gcc/ota_led_test.bin (link ở 0x08000000 như app thật).
 *
 * Vẫn chạy TaskComm để nhận "#ENTER_BOOTLOADER#" từ ESP32, nên sau khi nạp file này qua web
 * vẫn OTA lại được firmware.bin thật mà không cần ST-Link.
 *
 * Phân biệt trên kit (Blue Pill, LED PC13 sáng mức thấp):
 *   - Sáng liên tục   : app thử này đang chạy (OTA thành công).
 *   - Tắt             : app thật (firmware.bin) đang chạy.
 * Relay vẫn bị BSP ép OFF; không chạy I2C/OLED/INA219.
 */
#include "bsp.h"
#include "app_ipc.h"
#include "hal_gpio.h"
#include "hal_rcc.h"
#include "hal_uart.h"
#include "task_comm.h"
#include "task.h"

#define LED_PIN 13U

int main(void)
{
    bsp_system_init();                  /* relay OFF, nút input */

    RCC_EnableGPIOCClock();
    GPIO_WritePin(GPIOC, LED_PIN, 0U);  /* nạp mức sáng trước khi chuyển output */
    GPIO_ConfigPin(GPIOC, LED_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_2MHZ);

    if (app_ipc_init() != pdPASS) bsp_panic();
    Config_uart();
    if (task_comm_create() != pdPASS) bsp_panic();

    vTaskStartScheduler();
    bsp_panic();
}
