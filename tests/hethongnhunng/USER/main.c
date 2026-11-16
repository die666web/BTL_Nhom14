#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include <string.h>

/*==================================================
                    LCD CONFIG
==================================================*/

#define LCD_PORT GPIOB

#define LCD_RS GPIO_Pin_6
#define LCD_E  GPIO_Pin_5

#define LCD_D4 GPIO_Pin_15
#define LCD_D5 GPIO_Pin_14
#define LCD_D6 GPIO_Pin_13
#define LCD_D7 GPIO_Pin_12

/*==================================================
                    PASSWORD
==================================================*/

#define PASSWORD "1234"

/*==================================================
                    GLOBAL
==================================================*/

char entered[5];

uint8_t index_pass = 0;

/*==================================================
                    FUNCTION
==================================================*/

void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);

/* LCD */

void LCD_Init(void);
void LCD_Print(char *str);
void LCD_Gotoxy(uint8_t x, uint8_t y);
void LCD_Clear(void);
void LCD_Write4Bit(uint8_t val);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);

/* Servo */

void Servo_Init(void);
void Servo_WriteSpeed(int16_t speed);

/* Keypad */

void Keypad_Init(void);
char Keypad_GetKey(void);

/*==================================================
                KEYPAD CONNECTION
==================================================*/
/*
R1 -> PB11
R2 -> PB10
R3 -> PB1
R4 -> PB0

C1 -> PA4
C2 -> PA3
C3 -> PA2
C4 -> PA1
*/

/*==================================================
                KEYPAD MAP CHUAN
==================================================*/

char keypad[4][4] =
{
    {'1','4','7','*'},
    {'2','5','8','0'},
    {'3','6','9','#'},
    {'A','B','C','D'}
};

/*==================================================
                    MAIN
==================================================*/

int main(void)
{
    SystemInit();

    LCD_Init();

    Servo_Init();

    Keypad_Init();

    LCD_Clear();

    LCD_Gotoxy(0,0);

    LCD_Print("ENTER PASS:");

    while(1)
    {
        char key = Keypad_GetKey();

        if(key)
        {
            /*================ XOA PASSWORD =================*/

            if(key == '*')
            {
                index_pass = 0;

                LCD_Clear();

                LCD_Gotoxy(0,0);

                LCD_Print("ENTER PASS:");
            }

            /*================ NHAN ENTER =================*/

            else if(key == '#')
            {
                entered[index_pass] = '\0';

                if(strcmp(entered, PASSWORD) == 0)
                {
                    LCD_Clear();

                    LCD_Gotoxy(0,0);

                    LCD_Print("ACCESS OK");

                    LCD_Gotoxy(0,1);

                    LCD_Print("DOOR OPEN");

                    /* MO CUA */

                    Servo_WriteSpeed(60);

                    Delay_ms(500);

                    Servo_WriteSpeed(0);

                    Delay_ms(3000);

                    /* DONG CUA */

                    LCD_Clear();

                    LCD_Gotoxy(0,0);

                    LCD_Print("DOOR LOCK");

                    Servo_WriteSpeed(-60);

                    Delay_ms(500);

                    Servo_WriteSpeed(0);

                    Delay_ms(1000);
                }
                else
                {
                    LCD_Clear();

                    LCD_Gotoxy(0,0);

                    LCD_Print("WRONG PASS");

                    LCD_Gotoxy(0,1);

                    LCD_Print("TRY AGAIN");

                    Delay_ms(1500);
                }

                index_pass = 0;

                LCD_Clear();

                LCD_Gotoxy(0,0);

                LCD_Print("ENTER PASS:");
            }

            /*================ NHAP PASSWORD =================*/

           else
{
    if(index_pass < 4)
    {
        entered[index_pass++] = key;

        LCD_Gotoxy(index_pass - 1,1);

        LCD_SendData(key);
    }
}
        }
    }
}

/*==================================================
                    SERVO
==================================================*/

void Servo_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_TIM1,
                           ENABLE);

    GPIO_InitTypeDef GPIO_Servo;

    GPIO_Servo.GPIO_Pin = GPIO_Pin_8;

    GPIO_Servo.GPIO_Mode = GPIO_Mode_AF_PP;

    GPIO_Servo.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_Servo);

    TIM_TimeBaseInitTypeDef TIM_Base;

    TIM_Base.TIM_Prescaler = 72 - 1;

    TIM_Base.TIM_Period = 20000 - 1;

    TIM_Base.TIM_ClockDivision = 0;

    TIM_Base.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM1, &TIM_Base);

    TIM_OCInitTypeDef TIM_OC;

    TIM_OC.TIM_OCMode = TIM_OCMode_PWM1;

    TIM_OC.TIM_OutputState = TIM_OutputState_Enable;

    TIM_OC.TIM_Pulse = 1500;

    TIM_OC.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC1Init(TIM1, &TIM_OC);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    TIM_Cmd(TIM1, ENABLE);
}

void Servo_WriteSpeed(int16_t speed)
{
    uint16_t pulse;

    if(speed > 100) speed = 100;

    if(speed < -100) speed = -100;

    pulse = 1500 + speed * 5;

    TIM_SetCompare1(TIM1, pulse);
}

/*==================================================
                    KEYPAD
==================================================*/

void Keypad_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_GPIOA,
                           ENABLE);

    GPIO_InitTypeDef GPIO_Key;

    /* ROW OUTPUT */

    GPIO_Key.GPIO_Pin = GPIO_Pin_11 |
                        GPIO_Pin_10 |
                        GPIO_Pin_1  |
                        GPIO_Pin_0;

    GPIO_Key.GPIO_Mode = GPIO_Mode_Out_PP;

    GPIO_Key.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOB, &GPIO_Key);

    /* COLUMN INPUT */

    GPIO_Key.GPIO_Pin = GPIO_Pin_4 |
                        GPIO_Pin_3 |
                        GPIO_Pin_2 |
                        GPIO_Pin_1;

    GPIO_Key.GPIO_Mode = GPIO_Mode_IPD;

    GPIO_Key.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_Key);
}

char Keypad_GetKey(void)
{
    uint16_t rowPins[4] =
    {
        GPIO_Pin_0,
        GPIO_Pin_1,
        GPIO_Pin_10,
        GPIO_Pin_11
    };

    uint16_t colPins[4] =
    {
        GPIO_Pin_1,
        GPIO_Pin_2,
        GPIO_Pin_3,
        GPIO_Pin_4
    };

    int row, col;

    for(row = 0; row < 4; row++)
    {
        GPIO_ResetBits(GPIOB,
                       GPIO_Pin_11 |
                       GPIO_Pin_10 |
                       GPIO_Pin_1  |
                       GPIO_Pin_0);

        GPIO_SetBits(GPIOB, rowPins[row]);

        Delay_ms(1);

        for(col = 0; col < 4; col++)
        {
            if(GPIO_ReadInputDataBit(GPIOA, colPins[col]))
            {
                Delay_ms(20);

                if(GPIO_ReadInputDataBit(GPIOA, colPins[col]))
                {
                    char key = keypad[row][col];

                    Delay_ms(200);

                    return key;
                }
            }
        }
    }

    return 0;
}

/*==================================================
                    DELAY
==================================================*/

void Delay_us(uint32_t us)
{
    uint32_t count = us * 8;

    while(count--);
}

void Delay_ms(uint32_t ms)
{
    while(ms--)
    {
        Delay_us(1000);
    }
}

/*==================================================
                    LCD
==================================================*/

void LCD_Write4Bit(uint8_t val)
{
    (val & 0x01) ?
    GPIO_SetBits(LCD_PORT, LCD_D4) :
    GPIO_ResetBits(LCD_PORT, LCD_D4);

    (val & 0x02) ?
    GPIO_SetBits(LCD_PORT, LCD_D5) :
    GPIO_ResetBits(LCD_PORT, LCD_D5);

    (val & 0x04) ?
    GPIO_SetBits(LCD_PORT, LCD_D6) :
    GPIO_ResetBits(LCD_PORT, LCD_D6);

    (val & 0x08) ?
    GPIO_SetBits(LCD_PORT, LCD_D7) :
    GPIO_ResetBits(LCD_PORT, LCD_D7);

    GPIO_SetBits(LCD_PORT, LCD_E);

    Delay_us(30);

    GPIO_ResetBits(LCD_PORT, LCD_E);

    Delay_us(30);
}

void LCD_SendCommand(uint8_t cmd)
{
    GPIO_ResetBits(LCD_PORT, LCD_RS);

    LCD_Write4Bit(cmd >> 4);

    LCD_Write4Bit(cmd & 0x0F);

    Delay_ms(2);
}

void LCD_SendData(uint8_t data)
{
    GPIO_SetBits(LCD_PORT, LCD_RS);

    LCD_Write4Bit(data >> 4);

    LCD_Write4Bit(data & 0x0F);

    Delay_us(50);
}

void LCD_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,
                           ENABLE);

    GPIO_InitTypeDef GPIO_LCD;

    GPIO_LCD.GPIO_Pin = LCD_RS |
                        LCD_E  |
                        LCD_D4 |
                        LCD_D5 |
                        LCD_D6 |
                        LCD_D7;

    GPIO_LCD.GPIO_Mode = GPIO_Mode_Out_PP;

    GPIO_LCD.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(LCD_PORT, &GPIO_LCD);

    Delay_ms(50);

    LCD_Write4Bit(0x03);

    Delay_ms(5);

    LCD_Write4Bit(0x03);

    Delay_us(100);

    LCD_Write4Bit(0x03);

    LCD_Write4Bit(0x02);

    LCD_SendCommand(0x28);

    LCD_SendCommand(0x0C);

    LCD_SendCommand(0x06);

    LCD_Clear();
}

void LCD_Gotoxy(uint8_t x, uint8_t y)
{
    LCD_SendCommand((y == 0) ?
                    (0x80 + x) :
                    (0xC0 + x));
}

void LCD_Print(char *str)
{
    while(*str)
    {
        LCD_SendData(*str++);
    }
}

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);

    Delay_ms(2);
}