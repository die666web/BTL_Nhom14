#include <stdint.h>

#include "hal_spi.h"
#include "hal_rcc.h"
#include "hal_gpio.h"

#define SPI1_BASE 0x40013000UL

typedef struct
{
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} SPI_TypeDef;

#define SPI1 ((SPI_TypeDef *)SPI1_BASE)

void SPI1_Init(void)
{
    RCC_EnableGPIOAClock();
    RCC_EnableAPB2Clock(RCC_APB2_SPI1);

    /* PA5: SPI1 SCK, Alternate Function Push-Pull. */
    GPIO_ConfigPin(
        GPIOA,
        5,
        GPIO_MODE_AF_PP,
        GPIO_SPEED_50MHZ
    );

    /* PA7: SPI1 MOSI, Alternate Function Push-Pull. */
    GPIO_ConfigPin(
        GPIOA,
        7,
        GPIO_MODE_AF_PP,
        GPIO_SPEED_50MHZ
    );

    SPI1->CR1 = 0U;
    SPI1->CR2 = 0U;

    SPI1->CR1 =
          (1U << 2)   /* MSTR: Master mode */
        | (3U << 3)   /* BR=011: PCLK2 / 16 */
        | (1U << 8)   /* SSI */
        | (1U << 9);  /* SSM: Software slave management */

    /*
     * CPOL=0, CPHA=0: SPI Mode 0.
     * DFF=0: dữ liệu 8-bit.
     * LSBFIRST=0: gửi MSB trước.
     */

    SPI1->CR1 |= (1U << 6); /* SPE: bật SPI1 */
}

uint8_t SPI1_TransferByte(uint8_t data)
{
    uint8_t received;

    /* Chờ TX buffer trống. */
    while (!(SPI1->SR & (1U << 1)))
    {
    }

    SPI1->DR = data;

    /* Chờ nhận xong một byte. */
    while (!(SPI1->SR & (1U << 0)))
    {
    }

    received = (uint8_t)SPI1->DR;

    /* Chờ SPI truyền hoàn toàn. */
    while (SPI1->SR & (1U << 7))
    {
    }

    return received;
}