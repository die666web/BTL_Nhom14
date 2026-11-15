/* Xoá/ghi Flash nội STM32F103 (PM0075). Lê Trọng Mạnh. */
#include "boot_flash.h"
#include "boot_hw.h"
#include "ota_layout.h"

static bool in_writable(uint32_t addr, size_t len)
{
    return addr >= OTA_APP_BASE && addr <= OTA_FLASH_END && len <= OTA_FLASH_END - addr;
}

static void unlock(void)
{
    if (FLASH_CR & FLASH_CR_LOCK) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

static void lock(void) { FLASH_CR |= FLASH_CR_LOCK; }

/* Chờ xong, trả true nếu không có lỗi; xoá cờ EOP/PGERR/WRPRTERR (ghi 1 để xoá). */
static bool wait_done(void)
{
    while (FLASH_SR & FLASH_SR_BSY) { }
    uint32_t sr = FLASH_SR;
    FLASH_SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRT;
    return !(sr & (FLASH_SR_PGERR | FLASH_SR_WRPRT));
}

const uint8_t *boot_flash_ptr(uint32_t addr) { return (const uint8_t *)addr; }

bool boot_flash_erase_page(uint32_t page_addr)
{
    if ((page_addr % OTA_PAGE_SIZE) != 0u || !in_writable(page_addr, OTA_PAGE_SIZE)) return false;
    unlock();
    FLASH_SR = FLASH_SR_EOP | FLASH_SR_PGERR | FLASH_SR_WRPRT;
    FLASH_CR |= FLASH_CR_PER;
    FLASH_AR = page_addr;
    FLASH_CR |= FLASH_CR_STRT;
    bool ok = wait_done();
    FLASH_CR &= ~FLASH_CR_PER;
    lock();
    if (!ok) return false;
    const volatile uint32_t *p = (const volatile uint32_t *)page_addr;
    for (uint32_t i = 0; i < OTA_PAGE_SIZE / 4u; i++)
        if (p[i] != 0xFFFFFFFFu) return false;
    return true;
}

static bool program_hw_unlocked(uint32_t addr, uint16_t value)
{
    FLASH_CR |= FLASH_CR_PG;
    *(volatile uint16_t *)addr = value;
    bool ok = wait_done();
    FLASH_CR &= ~FLASH_CR_PG;
    return ok && *(volatile uint16_t *)addr == value;
}

bool boot_flash_program_u16(uint32_t addr, uint16_t value)
{
    if ((addr & 1u) || !in_writable(addr, 2u)) return false;
    unlock();
    bool ok = program_hw_unlocked(addr, value);
    lock();
    return ok;
}

bool boot_flash_program(uint32_t addr, const uint8_t *data, size_t len)
{
    if ((addr & 1u) || !in_writable(addr, (len + 1u) & ~(size_t)1u)) return false;
    bool ok = true;
    unlock();
    for (size_t i = 0; i < len && ok; i += 2u) {
        uint16_t hw = (uint16_t)(data[i] | ((i + 1u < len ? data[i + 1u] : 0xFFu) << 8));
        ok = program_hw_unlocked(addr + (uint32_t)i, hw);
    }
    lock();
    return ok;
}
