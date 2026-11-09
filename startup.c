#include <stdint.h>

/* =========================================================
   SYMBOLS TỪ LINKER SCRIPT
   ========================================================= */

#if !defined(__ARMCC_VERSION)
extern uint32_t _sidata;   // .data source trong FLASH
extern uint32_t _sdata;    // .data start trong RAM
extern uint32_t _edata;    // .data end trong RAM

extern uint32_t _sbss;     // .bss start
extern uint32_t _ebss;     // .bss end

extern uint32_t _estack;   // đỉnh Stack
#else
extern uint32_t Image$$ARM_LIB_STACK$$ZI$$Limit;
#define _estack Image$$ARM_LIB_STACK$$ZI$$Limit
#endif


/* =========================================================
   HANDLER PROTOTYPES
   ========================================================= */

void Reset_Handler(void);
void Default_Handler(void);


/* =========================================================
   CORE CORTEX-M3 EXCEPTIONS
   ========================================================= */

void NMI_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void HardFault_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void MemManage_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void BusFault_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void UsageFault_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void SVC_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void DebugMon_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void PendSV_Handler(void)
    __attribute__((weak, alias("Default_Handler")));

void SysTick_Handler(void)
    __attribute__((weak, alias("Default_Handler")));


/* =========================================================
   STM32F103 PERIPHERAL INTERRUPTS
   ========================================================= */

void WWDG_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void PVD_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void TAMPER_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void RTC_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void FLASH_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void RCC_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void EXTI0_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void EXTI1_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void EXTI2_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void EXTI3_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void EXTI4_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void DMA1_Channel1_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void DMA1_Channel2_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void DMA1_Channel3_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void DMA1_Channel4_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void DMA1_Channel5_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void DMA1_Channel6_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void DMA1_Channel7_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void ADC1_2_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void USB_HP_CAN1_TX_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void USB_LP_CAN1_RX0_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void CAN1_RX1_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void CAN1_SCE_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void EXTI9_5_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void TIM1_BRK_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void TIM1_UP_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void TIM1_TRG_COM_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void TIM1_CC_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void TIM2_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void TIM3_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void TIM4_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void I2C1_EV_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void I2C1_ER_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void I2C2_EV_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void I2C2_ER_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void SPI1_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void SPI2_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void USART1_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void USART2_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void USART3_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void EXTI15_10_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void RTCAlarm_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));

void USBWakeUp_IRQHandler(void)
    __attribute__((weak, alias("Default_Handler")));


/* =========================================================
   VECTOR TABLE
   ========================================================= */

__attribute__((section(".isr_vector"), used))
void (* const vector_table[])(void) =
{
    /* ================= CORE ================= */

    (void (*)(void))&_estack,   // 0  Stack Pointer
    Reset_Handler,              // 1
    NMI_Handler,                // 2
    HardFault_Handler,          // 3
    MemManage_Handler,          // 4
    BusFault_Handler,           // 5
    UsageFault_Handler,         // 6

    0,                          // 7 Reserved
    0,                          // 8 Reserved
    0,                          // 9 Reserved
    0,                          // 10 Reserved

    SVC_Handler,                // 11
    DebugMon_Handler,           // 12

    0,                          // 13 Reserved

    PendSV_Handler,             // 14
    SysTick_Handler,            // 15


    /* ================= STM32 IRQ ================= */

    WWDG_IRQHandler,            // IRQ 0
    PVD_IRQHandler,             // IRQ 1
    TAMPER_IRQHandler,          // IRQ 2
    RTC_IRQHandler,             // IRQ 3
    FLASH_IRQHandler,           // IRQ 4
    RCC_IRQHandler,             // IRQ 5

    EXTI0_IRQHandler,           // IRQ 6
    EXTI1_IRQHandler,           // IRQ 7
    EXTI2_IRQHandler,           // IRQ 8
    EXTI3_IRQHandler,           // IRQ 9
    EXTI4_IRQHandler,           // IRQ 10

    DMA1_Channel1_IRQHandler,   // IRQ 11
    DMA1_Channel2_IRQHandler,   // IRQ 12
    DMA1_Channel3_IRQHandler,   // IRQ 13
    DMA1_Channel4_IRQHandler,   // IRQ 14
    DMA1_Channel5_IRQHandler,   // IRQ 15
    DMA1_Channel6_IRQHandler,   // IRQ 16
    DMA1_Channel7_IRQHandler,   // IRQ 17

    ADC1_2_IRQHandler,          // IRQ 18

    USB_HP_CAN1_TX_IRQHandler,  // IRQ 19
    USB_LP_CAN1_RX0_IRQHandler,// IRQ 20
    CAN1_RX1_IRQHandler,        // IRQ 21
    CAN1_SCE_IRQHandler,        // IRQ 22

    EXTI9_5_IRQHandler,         // IRQ 23

    TIM1_BRK_IRQHandler,        // IRQ 24
    TIM1_UP_IRQHandler,         // IRQ 25
    TIM1_TRG_COM_IRQHandler,    // IRQ 26
    TIM1_CC_IRQHandler,         // IRQ 27

    TIM2_IRQHandler,            // IRQ 28
    TIM3_IRQHandler,            // IRQ 29
    TIM4_IRQHandler,            // IRQ 30

    I2C1_EV_IRQHandler,         // IRQ 31
    I2C1_ER_IRQHandler,         // IRQ 32
    I2C2_EV_IRQHandler,         // IRQ 33
    I2C2_ER_IRQHandler,         // IRQ 34

    SPI1_IRQHandler,            // IRQ 35
    SPI2_IRQHandler,            // IRQ 36

    USART1_IRQHandler,          // IRQ 37  <<< UART1 CỦA BẠN
    USART2_IRQHandler,          // IRQ 38
    USART3_IRQHandler,          // IRQ 39

    EXTI15_10_IRQHandler,       // IRQ 40
    RTCAlarm_IRQHandler,        // IRQ 41
    USBWakeUp_IRQHandler        // IRQ 42
};


/* =========================================================
   RESET HANDLER
   ========================================================= */

void Reset_Handler(void)
{
#if defined(__ARMCC_VERSION)
    /* Arm C runtime initializes scatter-loaded RW/ZI and calls main(). */
    extern void __main(void);
    __main();
#else
    uint32_t *src;
    uint32_t *dst;


    /* -----------------------------------------------------
       COPY .data

       Giá trị khởi tạo nằm trong FLASH
       -> copy sang RAM
       ----------------------------------------------------- */

    src = &_sidata;
    dst = &_sdata;

    while (dst < &_edata)
    {
        *dst++ = *src++;
    }


    /* -----------------------------------------------------
       CLEAR .bss

       Các global/static không khởi tạo phải = 0
       ----------------------------------------------------- */

    dst = &_sbss;

    while (dst < &_ebss)
    {
        *dst++ = 0;
    }


    /* -----------------------------------------------------
       CHẠY MAIN
       ----------------------------------------------------- */

    extern int main(void);

    main();
#endif


    /* main không được return */

    while (1)
    {
    }
}


/* =========================================================
   DEFAULT HANDLER
   ========================================================= */

void Default_Handler(void)
{
    /*
        Nếu interrupt xảy ra nhưng bạn chưa viết handler
        tương ứng thì CPU sẽ đứng ở đây.
    */

    while (1)
    {
    }
}