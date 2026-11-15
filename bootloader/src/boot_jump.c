/* Nhảy từ bootloader sang app tại OTA_APP_BASE. Lê Trọng Mạnh. */
#include "boot_jump.h"
#include "boot_hw.h"
#include "ota_layout.h"

void boot_jump_to_application(void)
{
    uint32_t msp   = *(const volatile uint32_t *)OTA_APP_BASE;
    uint32_t reset = *(const volatile uint32_t *)(OTA_APP_BASE + 4u);

    __asm volatile ("cpsid i" ::: "memory");

    SYST_CSR = 0u;                              /* tắt SysTick của bootloader */
    SYST_RVR = 0u;
    SYST_CVR = 0u;
    SCB_ICSR = SCB_ICSR_PENDSTCLR;
    for (uint32_t i = 0; i < 3u; i++) {         /* F103: 43 IRQ -> 2 thanh ghi, xoá dư cho chắc */
        NVIC_ICER(i) = 0xFFFFFFFFu;
        NVIC_ICPR(i) = 0xFFFFFFFFu;
    }

    SCB_VTOR = OTA_APP_BASE;                    /* app dùng bảng vector của chính nó */
    __asm volatile ("dsb\n\tisb" ::: "memory");

    /* Đặt MSP rồi nhảy trong cùng một khối asm: sau khi đổi MSP không được dùng stack cũ.
     * Bật lại PRIMASK để app thấy trạng thái giống sau reset (mọi IRQ đã tắt, không pending). */
    __asm volatile (
        "msr msp, %0\n\t"
        "cpsie i\n\t"
        "bx %1\n\t"
        :: "r"(msp), "r"(reset) : "memory");

    for (;;) { }
}
