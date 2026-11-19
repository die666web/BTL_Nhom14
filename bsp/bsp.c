#include "bsp.h"
#include "board_config.h"
#include "hal_gpio.h"
#include "hal_rcc.h"
#include <stdint.h>

/* Thanh ghi dùng riêng trong BSP (hal_rcc/hal_gpio chưa có). */
#define BSP_RCC_APB2RSTR  (*(volatile uint32_t *)0x4002100CUL)   /* reset ngoại vi APB2 */
#define BSP_RCC_APB1ENR   (*(volatile uint32_t *)0x4002101CUL)   /* bật clock ngoại vi APB1 */
#define BSP_PWR_CR        (*(volatile uint32_t *)0x40007000UL)
#define BSP_BKP_DR1       (*(volatile uint32_t *)0x40006C04UL)   /* giữ giá trị qua reset */
#define BSP_SCB_VTOR      (*(volatile uint32_t *)0xE000ED08UL)   /* địa chỉ bảng vector */
#define BSP_SCB_AIRCR     (*(volatile uint32_t *)0xE000ED0CUL)

#define BSP_RCC_APB1_BKP  (1UL << 27)
#define BSP_PWR_CR_DBP    (1UL << 8)    /* cho phép ghi vùng backup */

#define BSP_FLASH_BASE    0x08000000UL

/* Bootloader ROM do ST nạp sẵn trong chip (System Memory, tài liệu AN2606 / AN3155).
 * Word 0 = stack pointer ban đầu, word 1 = địa chỉ hàm bắt đầu của bootloader. */
#define BSP_SYSTEM_MEMORY_BASE  0x1FFFF000UL

#define BSP_RELAY_GPIO  ((GPIO_TypeDef *)BSP_RELAY_GPIO_BASE)
#define BSP_BUTTON_GPIO ((GPIO_TypeDef *)BSP_BUTTON_GPIO_BASE)


/* ================= Vùng backup (BKP_DR1) ================= */

/* Bật clock PWR + BKP và mở quyền ghi, để đọc/ghi BKP_DR1. */
static void backup_access_enable(void)
{
    BSP_RCC_APB1ENR |= RCC_APB1_PWR | BSP_RCC_APB1_BKP;
    (void)BSP_RCC_APB1ENR;                         /* đọc lại để chờ clock có hiệu lực */
    BSP_PWR_CR |= BSP_PWR_CR_DBP;
}

/* Đọc cờ "vào bootloader", rồi xoá ngay để lần reset sau chạy app bình thường. */
static uint8_t take_bootloader_flag(void)
{
    backup_access_enable();

    uint8_t requested = 0U;

    if ((BSP_BKP_DR1 & 0xFFFFUL) == BSP_BKP_FLAG_ENTER_BOOTLOADER)
    {
        requested = 1U;
    }

    BSP_BKP_DR1 = 0UL;

    return requested;
}


/* ================= Nhảy vào bootloader ROM của ST ================= */

/* Gọi ngay sau reset (chưa bật ngắt, chưa chạy FreeRTOS), relay đã OFF.
 * Bootloader ROM nghe USART1 (PA9/PA10), ESP32 nói chuyện với nó theo giao thức AN3155. */
static void jump_to_system_bootloader(void)
{
    uint32_t stack_pointer = *(volatile uint32_t *)BSP_SYSTEM_MEMORY_BASE;
    uint32_t entry_address = *(volatile uint32_t *)(BSP_SYSTEM_MEMORY_BASE + 4UL);

    __asm volatile (
        "msr msp, %0\n\t"
        "bx %1\n\t"
        :: "r"(stack_pointer), "r"(entry_address) : "memory");

    for (;;)
    {
    }
}


/* ================= API ================= */

void bsp_relay_force_off(void)
{
    GPIO_WritePin(BSP_RELAY_GPIO, BSP_RELAY_PIN, !BSP_RELAY_ACTIVE);
}

void bsp_system_init(void)
{
    /* Bước 1: bảng vector ở đầu Flash. Sau khi bootloader ROM chạy lệnh Go,
     * không chắc VTOR còn đúng nên đặt lại cho chắc. */
    BSP_SCB_VTOR = BSP_FLASH_BASE;

    /* Bước 2: relay OFF trước tiên. Ghi mức không tích cực TRƯỚC rồi mới chuyển chân
     * sang output, để relay không bị kích nhầm trong tích tắc. */
    RCC_EnableGPIOAClock();
    RCC_EnableGPIOBClock();
    bsp_relay_force_off();
    GPIO_ConfigPin(BSP_RELAY_GPIO, BSP_RELAY_PIN, GPIO_MODE_OUTPUT_PP, GPIO_SPEED_2MHZ);
    GPIO_ConfigPin(BSP_BUTTON_GPIO, BSP_BUTTON_PIN, GPIO_MODE_INPUT_PULLUP, GPIO_SPEED_INPUT);

    /* Bước 3: app vừa tự reset để cập nhật firmware: nhảy vào bootloader ROM (relay vẫn OFF). */
    if (take_bootloader_flag())
    {
        jump_to_system_bootloader();
    }

    /* Bước 4: bootloader ROM để lại USART1 ở chế độ 8 bit + parity chẵn.
     * Reset USART1 về mặc định để Config_uart() cấu hình lại từ đầu (8N1). */
    BSP_RCC_APB2RSTR |= RCC_APB2_USART1;
    BSP_RCC_APB2RSTR &= ~RCC_APB2_USART1;
}

void bsp_reboot_to_bootloader(void)
{
    __asm volatile ("cpsid i" ::: "memory");

    bsp_relay_force_off();

    /* Ghi cờ vào BKP_DR1 (không bị xoá khi reset), rồi reset bằng phần mềm.
     * Sau reset, bsp_system_init() thấy cờ và nhảy vào bootloader ROM. */
    backup_access_enable();
    BSP_BKP_DR1 = BSP_BKP_FLAG_ENTER_BOOTLOADER;

    __asm volatile ("dsb" ::: "memory");
    BSP_SCB_AIRCR = (0x05FAUL << 16) | (1UL << 2);      /* SYSRESETREQ */
    __asm volatile ("dsb" ::: "memory");

    for (;;)
    {
    }
}

void bsp_panic(void)
{
    __asm volatile ("cpsid i" ::: "memory");

    bsp_relay_force_off();

    for (;;)
    {
    }
}
