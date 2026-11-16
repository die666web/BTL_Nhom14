#include "stm32f10x.h"

#define BUTTON_PIN  (1UL << 0)   /* PA0 */
#define RELAY_PIN   (1UL << 12)  /* PB12 */
#define LED_PIN     (1UL << 13)  /* PC13 */
#define DEBOUNCE_MS 30U

typedef struct
{
    uint8_t last_raw;
    uint8_t stable;
    uint8_t stable_ms;
} ButtonState;

static void GPIO_Config(void)
{
    /* Bat clock GPIOA, GPIOB va GPIOC. */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN
                 | RCC_APB2ENR_IOPBEN
                 | RCC_APB2ENR_IOPCEN;
    (void)RCC->APB2ENR;

    /* PB12: nap 1 truoc de giu relay tat.
       Output open-drain, toc do 2 MHz. */
    GPIOB->BSRR = RELAY_PIN;
    GPIOB->CRH = (GPIOB->CRH & ~(0xFUL << 16))
               | (0x6UL << 16);

    /* PC13: output push-pull 2 MHz.
       LED tat luc khoi dong. */
    GPIOC->BSRR = LED_PIN;
    GPIOC->CRH = (GPIOC->CRH & ~(0xFUL << 20))
               | (0x2UL << 20);

    /* PA0: input pull-up.
       Tha nut = 1, nhan nut = 0. */
    GPIOA->BSRR = BUTTON_PIN;
    GPIOA->CRL = (GPIOA->CRL & ~0xFUL) | 0x8UL;
}

/* Bat/tat relay va LED bao trang thai. */
static void Fan_Set(uint8_t on)
{
    if (on != 0U)
    {
        GPIOB->BRR = RELAY_PIN;  /* Bat relay. */
        GPIOC->BRR = LED_PIN;    /* Sang LED PC13. */
    }
    else
    {
        GPIOB->BSRR = RELAY_PIN; /* Nha PB12: tat relay. */
        GPIOC->BSRR = LED_PIN;   /* Tat LED PC13. */
    }
}

/* Cau hinh bo dem SysTick 1 ms, khong bat ngat. */
static void Timer_Init(void)
{
    uint32_t ticks;

    SystemCoreClockUpdate();
    ticks = SystemCoreClock / 1000UL;

    SysTick->CTRL = 0UL;

    if ((ticks == 0UL) ||
        ((ticks - 1UL) > SysTick_LOAD_RELOAD_Msk))
    {
        while (1) { }
    }

    SysTick->LOAD = ticks - 1UL;
    SysTick->VAL = 0UL;

    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_ENABLE_Msk;
}

static void Delay_1ms(void)
{
    SysTick->VAL = 0UL;

    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0UL)
    {
    }
}

static uint8_t Button_Read(void)
{
    return ((GPIOA->IDR & BUTTON_PIN) != 0UL) ? 1U : 0U;
}

/* Goi moi 1 ms.
   Tra ve 1 mot lan khi nhan nut on dinh du 30 ms. */
static uint8_t Button_PressEvent(ButtonState *s, uint8_t raw)
{
    if (raw != s->last_raw)
    {
        s->last_raw = raw;
        s->stable_ms = 0U;
    }
    else if (s->stable_ms < DEBOUNCE_MS)
    {
        s->stable_ms++;

        if ((s->stable_ms == DEBOUNCE_MS) &&
            (raw != s->stable))
        {
            s->stable = raw;

            /* Chi tao su kien khi nhan, khong tao khi tha. */
            return (raw == 0U) ? 1U : 0U;
        }
    }

    return 0U;
}

int main(void)
{
    ButtonState button;
    uint8_t fan_on = 0U;

    GPIO_Config();
    Fan_Set(0U);
    Timer_Init();

    /* Giu nut khi cap nguon:
       can tha ra roi nhan lai de bat quat. */
    button.last_raw = Button_Read();
    button.stable = button.last_raw;
    button.stable_ms = 0U;

    while (1)
    {
        Delay_1ms();

        if (Button_PressEvent(&button, Button_Read()) != 0U)
        {
            fan_on ^= 1U;
            Fan_Set(fan_on);
        }
    }
}