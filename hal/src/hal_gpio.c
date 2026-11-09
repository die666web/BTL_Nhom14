#include <stdint.h>
#include "hal_gpio.h"

void GPIO_ConfigPin(GPIO_TypeDef *GPIOx, uint8_t pin,
                    GPIO_Mode mode, GPIO_Speed speed)
{
    volatile uint32_t *config_register;
    uint32_t shift;
    uint32_t cnf;
    uint32_t mode_bits;

    if (GPIOx == 0 || pin > 15U)
    {
        return;
    }

    if (mode >= GPIO_MODE_OUTPUT_PP &&
        (speed < GPIO_SPEED_10MHZ || speed > GPIO_SPEED_50MHZ)) return;

    if (pin < 8U)
    {
        config_register = &GPIOx->CRL;
        shift = (uint32_t)pin * 4U;
    }
    else
    {
        config_register = &GPIOx->CRH;
        shift = ((uint32_t)pin - 8U) * 4U;
    }

    cnf = 0U;
    mode_bits = 0U;

    switch (mode)
    {
        case GPIO_MODE_ANALOG:
            break;
        case GPIO_MODE_INPUT_FLOATING:
            cnf = 1U;
            break;
        case GPIO_MODE_INPUT_PULLUP:
            cnf = 2U;
            GPIOx->BSRR = (1U << pin);
            break;
        case GPIO_MODE_INPUT_PULLDOWN:
            cnf = 2U;
            GPIOx->BRR = (1U << pin);
            break;
        case GPIO_MODE_OUTPUT_PP:
            mode_bits = speed;
            break;
        case GPIO_MODE_OUTPUT_OD:
            cnf = 1U;
            mode_bits = speed;
            break;
        case GPIO_MODE_AF_PP:
            cnf = 2U;
            mode_bits = speed;
            break;
        case GPIO_MODE_AF_OD:
            cnf = 3U;
            mode_bits = speed;
            break;
        default:
            return;
    }

    *config_register &= ~(0xFU << shift);
    *config_register |= (((cnf << 2) | mode_bits) << shift);
}

void GPIO_WritePin(GPIO_TypeDef *GPIOx, uint8_t pin, uint8_t state)
{
    if (GPIOx == 0 || pin > 15U)
    {
        return;
    }

    if (state)
    {
        GPIOx->BSRR = (1U << pin);
    }
    else
    {
        GPIOx->BRR = (1U << pin);
    }
}

uint8_t GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint8_t pin)
{
    if (GPIOx == 0 || pin > 15U)
    {
        return 0;
    }

    return (uint8_t)((GPIOx->IDR >> pin) & 1U);
}

