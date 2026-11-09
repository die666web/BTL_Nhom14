#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "hal_uart.h"
#include "hal_gpio.h"
#include "hal_dma.h"
#include "board_config.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

/* Software queues and mmap register storage ONLY. No emulation of peripheral
 * timing, interrupt preemption, DMA or FreeRTOS scheduling. */
struct MockQueue { size_t count, width, used, head, tail; unsigned char *data; };
struct MockMutex { int locked; };
static unsigned assertions;
#define CHECK(x) do { assert(x); ++assertions; } while (0)
static TickType_t now;
static int scheduler = taskSCHEDULER_RUNNING;
TickType_t xTaskGetTickCount(void) { return now; }
BaseType_t xTaskGetSchedulerState(void) { return scheduler; }
QueueHandle_t xQueueCreate(UBaseType_t count, UBaseType_t size)
{
    QueueHandle_t q=calloc(1,sizeof(*q)); assert(q);
    q->count=count; q->width=size; q->data=calloc(count,size); assert(q->data); return q;
}
void vQueueDelete(QueueHandle_t q) { free(q->data);free(q); }
BaseType_t xQueueSend(QueueHandle_t q,const void *data,TickType_t timeout)
{
    if (q->used==q->count) { now+=timeout; return pdFAIL; }
    memcpy(q->data+q->head*q->width,data,q->width);
    q->head=(q->head+1)%q->count; ++q->used; return pdPASS;
}
BaseType_t xQueueReceive(QueueHandle_t q,void *data,TickType_t timeout)
{
    if (!q->used) { now+=timeout; return pdFAIL; }
    memcpy(data,q->data+q->tail*q->width,q->width);
    q->tail=(q->tail+1)%q->count; --q->used; return pdPASS;
}
BaseType_t xQueueSendFromISR(QueueHandle_t q,const void *data,BaseType_t *wake)
{ (void)wake;return xQueueSend(q,data,0); }
BaseType_t xQueueReceiveFromISR(QueueHandle_t q,void *data,BaseType_t *wake)
{ (void)wake;return xQueueReceive(q,data,0); }
SemaphoreHandle_t xSemaphoreCreateMutex(void) { return calloc(1,sizeof(struct MockMutex)); }
BaseType_t xSemaphoreTake(SemaphoreHandle_t m,TickType_t timeout)
{ if(m->locked) { now+=timeout;return pdFAIL; } m->locked=1;return pdPASS; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t m) { m->locked=0;return pdPASS; }
void vSemaphoreDelete(SemaphoreHandle_t m) { free(m); }
static volatile uint32_t *uart=(volatile uint32_t *)0x40013800UL;
static void map_region(uintptr_t address,size_t size)
{
    void *ptr=mmap((void *)address,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED,-1,0);
    assert(ptr!=(void *)-1);
}
static void feed(unsigned char c,uint32_t flags)
{ uart[0]=flags;uart[1]=c;USART1_IRQHandler(); }
static void feed_text(const char *s) { while(*s) feed((unsigned char)*s++,1U<<5); }
static size_t drain(char *out,size_t capacity)
{
    size_t n=0;
    while(uart[3]&(1U<<7)) {
        uart[0]=1U<<7; USART1_IRQHandler();
        if(uart[3]&(1U<<7)) { assert(n+1<capacity);out[n++]=(char)uart[1]; }
    }
    out[n]=0;return n;
}
int main(void)
{
    map_region(0x40000000UL,0x30000U);map_region(0xE000E000UL,0x1000U);
    CHECK(hal_uart_write((const uint8_t *)"x",1,1)==HAL_NOT_READY);
    CHECK(Config_uart()==pdPASS);
    CHECK(uart[2]==833U); CHECK(uart[5]==0U);
    CHECK((*(volatile uint32_t *)0xE000E104UL)==(1U<<5));
    CHECK(((volatile uint8_t *)0xE000E400UL)[37]==0x60);
    CHECK(Config_uart()==pdFAIL);
    char out[512],expected[8];
    /* Full uint8 range catches the old 101..255 formatting defect. */
    for(unsigned n=0;n<=255;++n) {
        snprintf(expected,sizeof(expected),"%u",n);
        CHECK(SendNumberInterrupt_UART((uint8_t)n)==HAL_OK);
        drain(out,sizeof(out)); CHECK(strcmp(out,expected)==0);
    }
    CHECK(SendStringInterrupt_UART("abc")==HAL_OK);drain(out,sizeof(out));CHECK(strcmp(out,"abc\n")==0);
    CHECK(SendTextInterrupt_UART(NULL)==HAL_INVALID_ARGUMENT);
    CHECK(hal_uart_write(NULL,1,0)==HAL_INVALID_ARGUMENT);
    CHECK(hal_uart_write((const uint8_t *)"a",1,portMAX_DELAY)==HAL_INVALID_ARGUMENT);
    scheduler=0;CHECK(SendTextInterrupt_UART("a")==HAL_NOT_READY);scheduler=taskSCHEDULER_RUNNING;
    uint8_t bytes[200]={0};
    CHECK(hal_uart_write(bytes,sizeof(bytes),10)==HAL_TIMEOUT);
    CHECK(drain(out,sizeof(out))==128U);
    CHECK(SendTextInterrupt_UART("after timeout")==HAL_OK);drain(out,sizeof(out));CHECK(strcmp(out,"after timeout")==0);
    char line[16]="guard";
    feed_text("HELLO\r\n");CHECK(UART_ReadLine(line,sizeof(line))==1);CHECK(strcmp(line,"HELLO")==0);
    CHECK(UART_ReadLine(line,sizeof(line))==0);
    feed_text("X\n");CHECK(UART_ReadLine(line,0)==0);CHECK(UART_ReadLine(line,1)==0);
    CHECK(UART_ReadLine(NULL,16)==0);CHECK(UART_ReadLine(line,sizeof(line))==1);CHECK(strcmp(line,"X")==0);
    feed_text("ABCDE\n");CHECK(UART_ReadLine(line,3)==0);
    for(unsigned i=0;i<140;i++) { feed('Q',1U<<5);(void)UART_ReadLine(line,sizeof(line)); }
    feed_text("\nON\n");CHECK(UART_ReadLine(line,sizeof(line))==1);CHECK(strcmp(line,"ON")==0);
    uint32_t errors=hal_uart_rx_error_count();feed('!',1U<<3);
    CHECK(hal_uart_rx_error_count()==errors+1);CHECK(ReceiveInterrupt_UART(bytes)==0);
    CHECK(ReceiveInterrupt_UART(NULL)==0);
    for(unsigned i=0;i<130;i++) feed('a',1U<<5);
    CHECK(hal_uart_rx_error_count()==errors+3);
    while(ReceiveInterrupt_UART(bytes)) { }
    CHECK(StringEqual("same","same"));CHECK(!StringEqual("a","b"));CHECK(!StringEqual(NULL,"a"));

    GPIOB->CRH=0x44444444U;
    GPIO_ConfigPin(GPIOB,8,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_INPUT);
    CHECK(GPIOB->CRH==0x44444444U);
    GPIO_ConfigPin(GPIOB,8,GPIO_MODE_OUTPUT_PP,GPIO_SPEED_2MHZ);
    CHECK(GPIOB->CRH==0x44444442U);
    GPIO_ConfigPin(GPIOB,7,GPIO_MODE_INPUT_PULLUP,GPIO_SPEED_INPUT);
    CHECK(((GPIOB->CRL>>28)&15U)==8U);CHECK(GPIOB->BSRR==(1U<<7));
    GPIO_WritePin(GPIOB,8,1);CHECK(GPIOB->BSRR==(1U<<8));
    GPIO_WritePin(GPIOB,8,0);CHECK(GPIOB->BRR==(1U<<8));

    dma_config cfg={.direction=DMA_Direc_MemoryToPeriph,.memory_size=DMA_SIZE_8Bit,
        .periph_size=DMA_SIZE_8Bit,.priority=DMA_Priority_HIGH,.minc=1,.complete_interrupt=1};
    DMA1_Channel4->CCR=0xFFFFU;
    DMA_Init(DMA1_Channel4,(void *)0x40013804UL,&cfg);
    CHECK(DMA1_Channel4->CCR==((1U<<4)|(2U<<12)|(1U<<7)|(1U<<1)));
    CHECK(DMA1->IFCR==(15U<<12));CHECK(DMA1_Channel4->CPAR==0x40013804UL);
    DMA_Start(DMA1_Channel4,(void *)0x20000100UL,10);
    CHECK(DMA1_Channel4->CCR&1U);CHECK(DMA1_Channel4->CNDTR==10);
    CHECK(DMA1_Channel4->CMAR==0x20000100UL);CHECK(DMA1->IFCR==(15U<<12));
    DMA_Stop(DMA1_Channel4);CHECK(!(DMA1_Channel4->CCR&1U));
    uint32_t old=DMA1_Channel4->CCR;
    cfg.memory_size=(DMA_Datasize)3;DMA_Init(DMA1_Channel4,(void *)0x40013804UL,&cfg);
    CHECK(DMA1_Channel4->CCR==old);
    DMA_Init(NULL,NULL,NULL);DMA_Start(NULL,NULL,10);DMA_Stop(NULL);CHECK(DMA_GetRemainingData(NULL)==0);
    DMA1->IFCR=123;DMA_ClearAllFlag(0);CHECK(DMA1->IFCR==123);
    printf("PASS: %u assertions; UART formatting/queues/line bounds/errors, GPIO, DMA.\n",assertions);
    return 0;
}
