#include "hal_dma.h"
#include "hal_rcc.h"

static uint8_t dma_channel_number(DMA_Channel_TypeDef *channel)
{
    uintptr_t address = (uintptr_t)channel;
    uintptr_t first = DMA1_BASE + 0x08U;
    if (address < first || address > DMA1_BASE + 0x80U ||
        (address - first) % 0x14U != 0U) return 0;
    return (uint8_t)((address - first) / 0x14U + 1U);
}

void DMA_Init(
    DMA_Channel_TypeDef *Channel,
    volatile void *periph_address,
    dma_config *config
)
{
    uint8_t number = dma_channel_number(Channel);
    if (!number || !periph_address || !config) return;
    if ((unsigned)config->direction > 1U || (unsigned)config->memory_size > 2U ||
        (unsigned)config->periph_size > 2U || (unsigned)config->priority > 3U ||
        (unsigned)config->mode > 1U || config->minc > 1U || config->pinc > 1U ||
        config->complete_interrupt > 1U || config->half_interrupt > 1U ||
        config->error_interrupt > 1U || config->memory_to_memory > 1U ||
        (config->memory_to_memory && config->mode == DMA_MODE_CIRCULAR)) return;
    RCC_EnableAHBClock(RCC_AHB_DMA1);

    Channel->CCR &= ~1U;
    Channel->CCR = 0U;
    DMA_ClearAllFlag(number);
    Channel->CCR |= config->direction << 4
                | config->memory_size << 10
                | config->periph_size << 8
                | config->priority << 12
                | config->minc << 7
                | config->pinc << 6
                | config->mode << 5
                | config->memory_to_memory << 14
                | config->complete_interrupt << 1
                | config->half_interrupt << 2
                | config->error_interrupt << 3
                ;
    
    Channel->CPAR = (uint32_t)(uintptr_t)periph_address;
}


void DMA_Start
(
    DMA_Channel_TypeDef *Channel,
    void *memory_address,
    uint16_t number_data
)
{
    uint8_t number = dma_channel_number(Channel);
    if (!number || !memory_address || number_data == 0) return;
    Channel->CCR &= ~(1U << 0); // Tat trc khi cau hinh
    DMA_ClearAllFlag(number);
    Channel->CNDTR = number_data; // cau hinh thong so 
    Channel->CMAR = (uint32_t)(uintptr_t)memory_address; // cau hinh vi tri trong ram de su dung
    Channel->CCR |= 1U << 0; // Bat
}

void DMA_Stop(DMA_Channel_TypeDef *Channel)
{
    if (!dma_channel_number(Channel)) return;
    Channel->CCR &= ~(1U << 0);
}

uint8_t DMA_IsHalfTransfer(uint8_t channel_number)
{
    if (channel_number < 1U || channel_number > 7U)
    {
        return 0;
    }
    
    if(DMA1->ISR & (1U << ((channel_number - 1) * 4 + 2))) return 1;
    return 0;
}

uint8_t DMA_IsFullTransfer(uint8_t channel_number)
{
    if (channel_number < 1U || channel_number > 7U)
    {
        return 0;
    }
    if(DMA1->ISR & (1U << ((channel_number - 1) * 4 + 1))) return 1;
    return 0;
}

uint8_t DMA_IsErrorTransfer(uint8_t channel_number)
{
    if (channel_number < 1U || channel_number > 7U)
    {
        return 0;
    }
    if(DMA1->ISR & (1U << ((channel_number - 1) * 4 + 3))) return 1;
    return 0;
}

void DMA_ClearFlagHalfTransfer(uint8_t channel_number)
{
    if (channel_number < 1U || channel_number > 7U)
    {
        return;
    }
    DMA1->IFCR = 1U << ((channel_number - 1)* 4 + 2);
}

void DMA_ClearFlagFullTransfer(uint8_t channel_number)
{
    if (channel_number < 1U || channel_number > 7U)
    {
        return;
    }
    DMA1->IFCR = 1U << ((channel_number - 1) * 4 + 1);
}

void DMA_ClearFlagError(uint8_t channel_number)
{
    if (channel_number < 1U || channel_number > 7U)
    {
        return;
    }
    DMA1->IFCR = 1U << ((channel_number - 1) * 4 + 3);
}

void DMA_ClearAllFlag(uint8_t channel_number)
{
    if (channel_number < 1U || channel_number > 7U)
    {
        return;
    }
    DMA1->IFCR = 0xFU << ((channel_number - 1) * 4);
}

uint16_t DMA_GetRemainingData(DMA_Channel_TypeDef *Channel)
{
    if (!dma_channel_number(Channel)) return 0;
    return (uint16_t)Channel->CNDTR;
}