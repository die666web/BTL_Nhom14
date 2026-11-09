/* DMA channel has one owner. Configure/init before use; serialize Start/Stop
 * within that owner. Not used by baseline UART (interrupt queues).
 * DMA API is low-level, not a shared multi-task transfer service. */
#ifndef DMA_H
#define DMA_H 

#include "stdint.h"
/**
 * @brief Cac thong so 1 channel co the lam
 * 1. Huong truyen:
 *  - Memory -> Periph
 *  - Periph -> Memory
 * 2. Data sie:
 *  - 8bit cho UART
 *  - 16bit cho ADc
 *  - 32 bit neu can du lieu lon hon
 * 3. Muc do lay mau
 *  - Binh thuong: lay mau tung lan mot
 *  - LAy mau lien tuc
 * 4. Muc do uu tien
 *  - Uu tien 1,2,3,4
 * 5. Bat tat DMA
 * 
 * # MINC: tang dia chi memory: 1 do hay phai tang de doc dia chi bo nho 
 * # PINC: tang dia chi ngoai vi: thuong se giu nguyen, boi thuong hay dung 1 thanh gh
 */

typedef enum
{
    DMA_Direc_PeriphToMemory = 0,
    DMA_Direc_MemoryToPeriph = 1,
    
} DMA_Direction;

typedef enum 
{
    DMA_SIZE_8Bit = 0,
    DMA_SIZE_16Bit = 1,
    DMA_SIZE_32Bit = 2,
} DMA_Datasize;

typedef enum
{
    DMA_MODE_NORMAL = 0,
    DMA_MODE_CIRCULAR = 1,
} DMA_Mode;

typedef enum 
{
    DMA_Priority_LOW = 0,
    DMA_Priority_MED = 1,
    DMA_Priority_HIGH = 2,
    DMA_Priority_VERHIGH = 3,
} DMA_Priority;

typedef enum
{
    DMA_Disable = 0,
    DMA_Enable = 1,
} DMA_State;

#define DMA1_BASE 0x40020000

typedef struct
{
    volatile uint32_t CCR;    // 0x00: cấu hình
    volatile uint32_t CNDTR;  // 0x04: số dữ liệu
    volatile uint32_t CPAR;   // 0x08: địa chỉ ngoại vi
    volatile uint32_t CMAR;   // 0x0C: địa chỉ RAM
} DMA_Channel_TypeDef;


// TCIF: Bat khi da truyen xong 
// HTIF: Bat len khi da truyen duoc 1 nuwa
// TEIF: Bat len khi truyen hong
// GIF: Bat len khi 1 trong 3 co tren bat
// Dao nguoc voi ifcr
// Moi thanh ghi 32 bit: 4 bit cho tung channel, tong laf 7 channel , conf  du 5 bit cho reserved

typedef struct
{
    volatile uint32_t ISR; // Chi co kha nang doc, dung de xem trang thai
    volatile uint32_t IFCR; // Chi co kha nang ghi, dung de xoa
} DMA_TypeDef;


#define DMA1 ((DMA_TypeDef *)DMA1_BASE)

// Khoang casch cac channel laf 20byte
// Moi channel co 4 thanh ghi: moi thanh ghi 4 byte => Tong 16 byte + 1 vungf dem reserved 4 byte = 20 bye
// 1C - 08 = 0x14 : tuong ung 20byte

#define DMA1_Channel1 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x08U))
#define DMA1_Channel2 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x1CU))
#define DMA1_Channel3 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x30U))
#define DMA1_Channel4 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x44U))
#define DMA1_Channel5 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x58U))
#define DMA1_Channel6 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x6CU))
#define DMA1_Channel7 ((DMA_Channel_TypeDef *)(DMA1_BASE + 0x80U))
    

/**
 * @brief Huong dan dien bang dm_config
 * Direction 
 * Mode 
 * Priority
 * Minc
 * Pinc
 * Half_interrupt
 * Complete_interrupt
 */

typedef struct
{
    DMA_Direction direction; // Huong memory -> per hoac nguoc lai
    DMA_Datasize periph_size; // 8, 16, 32
    DMA_Datasize memory_size; // 8, 16, 32
    DMA_Mode mode; // lien tuc hay laf tung lan 1 
    DMA_Priority priority; // uu tien 1 -> 2 -> 3 -> 4
    
    uint8_t minc; // Co dem tang memory ko 
    uint8_t pinc; // co dem tang dia chi thanh ghi ko 
    uint8_t half_interrupt; // co thuc hien ngat khi chay duoc1 nua du lieu ko 
    uint8_t complete_interrupt; // co thuc hien ngat khi chay full du lieu ko 
    uint8_t error_interrupt; // ngat khi co loi xay ra
    uint8_t memory_to_memory; // thuc hien truyen tu memory -> memory
} dma_config;

/**
 * @brief Huong dan dien config
 * 
 * @param Channel 
 * @param periph_address 
 * @param memory_address 
 * @param number_data 
 * @param dma_config 
 */

void DMA_Init(
    DMA_Channel_TypeDef *Channel,
    volatile void *periph_address,
    dma_config *config
);

void DMA_Start
(
    DMA_Channel_TypeDef *Channel,
    void *memory_address,
    uint16_t number_data
);

void DMA_Stop(DMA_Channel_TypeDef *Channel);

uint8_t DMA_IsHalfTransfer(uint8_t channel_number);
uint8_t DMA_IsFullTransfer(uint8_t channel_number);
uint8_t DMA_IsErrorTransfer(uint8_t channel_number);

void DMA_ClearFlagHalfTransfer(uint8_t channel_number);
void DMA_ClearFlagFullTransfer(uint8_t channel_number);
void DMA_ClearFlagError(uint8_t channel_number);
void DMA_ClearAllFlag(uint8_t channel_number);
uint16_t DMA_GetRemainingData(DMA_Channel_TypeDef *Channel);


#endif