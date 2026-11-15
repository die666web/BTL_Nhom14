/* Phần cứng bootloader: SysTick, USART1 polling, LED PC13, relay OFF, cờ BKP. Lê Trọng Mạnh. */
#include "boot_port.h"
#include "boot_hw.h"
#include "board_config.h"
#include "ota_layout.h"

#define BOOT_UART_BAUD 115200u   /* phải khớp BSP_UART_BAUD của app và ESP32 */

static volatile uint32_t s_ms;

void SysTick_Handler(void) { s_ms++; }

static void gpio_mode(uint32_t base, uint32_t pin, uint32_t mode4)
{
    volatile uint32_t *cr = (pin < 8u) ? &GPIO_CRL(base) : &GPIO_CRH(base);
    uint32_t sh = (pin & 7u) * 4u;
    *cr = (*cr & ~(0xFu << sh)) | (mode4 << sh);
}

bool port_take_update_flag(void)
{
    RCC_APB1ENR |= RCC_APB1_PWR | RCC_APB1_BKP;
    (void)RCC_APB1ENR;
    PWR_CR |= PWR_CR_DBP;
    bool flag = (BKP_DR1 & 0xFFFFu) == OTA_BKP_FLAG_UPDATE;
    BKP_DR1 = 0u;                         /* đọc rồi xoá ngay */
    PWR_CR &= ~PWR_CR_DBP;
    RCC_APB1ENR &= ~(RCC_APB1_PWR | RCC_APB1_BKP);
    return flag;
}

void port_init(void)
{
    /* Clock: giữ HSI 8 MHz sau reset, giống app. */
    RCC_APB2ENR |= RCC_APB2_IOPA | RCC_APB2_IOPB | RCC_APB2_IOPC | RCC_APB2_USART1;
    (void)RCC_APB2ENR;

    /* Relay OFF: ghi mức không tích cực rồi mới chuyển output (giống bsp_system_init). */
    GPIO_BSRR(GPIOB_BASE) = BSP_RELAY_ACTIVE ? (1u << (BSP_RELAY_PIN + 16u)) : (1u << BSP_RELAY_PIN);
    gpio_mode(GPIOB_BASE, BSP_RELAY_PIN, 0x2u);       /* output push-pull 2 MHz */

    /* LED PC13 (Blue Pill, sáng mức thấp) */
    GPIO_BSRR(GPIOC_BASE) = 1u << 13;
    gpio_mode(GPIOC_BASE, 13u, 0x2u);

    /* USART1: PA9 AF push-pull 50 MHz, PA10 input floating */
    gpio_mode(GPIOA_BASE, BSP_UART_TX_PIN, 0xBu);
    gpio_mode(GPIOA_BASE, BSP_UART_RX_PIN, 0x4u);
    USART1_CR1 = 0u;
    USART1_CR2 = 0u;
    USART1_CR3 = 0u;
    USART1_BRR = (BSP_APB2_CLOCK_HZ + BOOT_UART_BAUD / 2u) / BOOT_UART_BAUD;
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    (void)USART1_SR;
    (void)USART1_DR;

    /* SysTick 1 ms từ HCLK */
    SYST_RVR = BSP_CPU_CLOCK_HZ / 1000u - 1u;
    SYST_CVR = 0u;
    SYST_CSR = 0x7u;                                   /* CLKSOURCE | TICKINT | ENABLE */
}

uint32_t port_ms(void) { return s_ms; }

int port_uart_getc(uint8_t *byte)
{
    uint32_t sr = USART1_SR;
    if (!(sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)))
        return 0;
    *byte = (uint8_t)USART1_DR;                        /* đọc SR rồi DR: xoá RXNE + cờ lỗi */
    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE))
        return -1;
    return 1;
}

void port_uart_write(const uint8_t *data, size_t len)
{
    while (len--) {
        while (!(USART1_SR & USART_SR_TXE)) { }
        USART1_DR = *data++;
    }
    while (!(USART1_SR & USART_SR_TC)) { }
}

void port_led(bool on)
{
    GPIO_BSRR(GPIOC_BASE) = on ? (1u << (13u + 16u)) : (1u << 13);
}

void port_deinit(void)
{
    port_led(false);
    while (!(USART1_SR & USART_SR_TC)) { }
    USART1_CR1 = 0u;
    /* Reset USART1, GPIOA, GPIOC về mặc định. GPIOB giữ nguyên để relay vẫn OFF tới khi app tự cấu hình. */
    RCC_APB2RSTR |= RCC_APB2_USART1 | RCC_APB2_IOPA | RCC_APB2_IOPC;
    RCC_APB2RSTR &= ~(RCC_APB2_USART1 | RCC_APB2_IOPA | RCC_APB2_IOPC);
    RCC_APB2ENR &= ~(RCC_APB2_USART1 | RCC_APB2_IOPA | RCC_APB2_IOPC);
}
