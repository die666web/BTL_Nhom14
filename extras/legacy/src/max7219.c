#include <stdint.h>

#include "max7219.h"
#include "hal_spi.h"
#include "hal_gpio.h"

#define MAX7219_CS_PIN 4U

#define MAX7219_REG_DECODE_MODE  0x09U
#define MAX7219_REG_INTENSITY    0x0AU
#define MAX7219_REG_SCAN_LIMIT   0x0BU
#define MAX7219_REG_SHUTDOWN     0x0CU
#define MAX7219_REG_DISPLAY_TEST 0x0FU

static void MAX7219_Write(
    uint8_t address,
    uint8_t data
)
{
    /* CS xuống thấp: bắt đầu truyền. */
    GPIO_WritePin(GPIOA, MAX7219_CS_PIN, 0U);

    SPI1_TransferByte(address);
    SPI1_TransferByte(data);

    /* CS lên cao: MAX7219 chốt 16-bit dữ liệu. */
    GPIO_WritePin(GPIOA, MAX7219_CS_PIN, 1U);
}

void MAX7219_Clear(void)
{
    uint8_t digit;

    for (digit = 1U; digit <= 8U; digit++)
    {
        /* 0x0F là blank khi dùng Code-B Decode. */
        MAX7219_Write(digit, 0x0FU);
    }
}

void MAX7219_SetIntensity(uint8_t intensity)
{
    if (intensity > 15U)
    {
        intensity = 15U;
    }

    MAX7219_Write(
        MAX7219_REG_INTENSITY,
        intensity
    );
}

void MAX7219_Init(void)
{
    SPI1_Init();

    GPIO_ConfigPin(
        GPIOA,
        MAX7219_CS_PIN,
        GPIO_MODE_OUTPUT_PP,
        GPIO_SPEED_50MHZ
    );

    /* Trạng thái mặc định của CS là mức cao. */
    GPIO_WritePin(GPIOA, MAX7219_CS_PIN, 1U);

    /* Tắt Display Test. */
    MAX7219_Write(
        MAX7219_REG_DISPLAY_TEST,
        0U
    );

    /* Bật Code-B Decode cho cả 8 digit. */
    MAX7219_Write(
        MAX7219_REG_DECODE_MODE,
        0xFFU
    );

    /* Sử dụng đủ digit 1 đến digit 8. */
    MAX7219_Write(
        MAX7219_REG_SCAN_LIMIT,
        7U
    );

    /* Độ sáng mức thấp. */
    MAX7219_SetIntensity(2U);

    /* Thoát Shutdown, bật hiển thị. */
    MAX7219_Write(
        MAX7219_REG_SHUTDOWN,
        1U
    );

    MAX7219_Clear();
}

void MAX7219_DisplayNumber(uint32_t number)
{
    uint8_t digit = 1U;

    MAX7219_Clear();

    if (number == 0U)
    {
        MAX7219_Write(1U, 0U);
        return;
    }

    while (number > 0U && digit <= 8U)
    {
        MAX7219_Write(
            digit,
            (uint8_t)(number % 10U)
        );

        number /= 10U;
        digit++;
    }
}