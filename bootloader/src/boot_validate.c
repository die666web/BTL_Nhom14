/* Metadata + kiểm tra ảnh app. Thuần logic trên boot_flash.h (chạy được cả trên PC test). */
#include "boot_validate.h"
#include "boot_flash.h"
#include "ota_frame.h"
#include "ota_layout.h"

static uint32_t rd32(uint32_t addr) { return ota_get_u32(boot_flash_ptr(addr)); }
static uint16_t rd16(uint32_t addr) { return ota_get_u16(boot_flash_ptr(addr)); }

void boot_meta_read(boot_meta_t *m)
{
    m->size        = rd32(OTA_META_BASE + 0u);
    m->image_crc32 = rd32(OTA_META_BASE + 4u);
    m->valid       = rd16(OTA_META_BASE + 8u);
}

bool boot_meta_invalidate(void)
{
    return boot_flash_erase_page(OTA_META_BASE) && rd16(OTA_META_BASE + 8u) == 0xFFFFu;
}

bool boot_meta_commit(uint32_t size, uint32_t image_crc32)
{
    uint8_t b[8];
    ota_put_u32(&b[0], size);
    ota_put_u32(&b[4], image_crc32);
    if (!boot_flash_program(OTA_META_BASE, b, sizeof b)) return false;
    if (rd32(OTA_META_BASE) != size || rd32(OTA_META_BASE + 4u) != image_crc32) return false;
    /* valid ghi sau cùng: mất điện trước bước này thì app chưa bao giờ được coi là hợp lệ */
    return boot_flash_program_u16(OTA_META_BASE + 8u, OTA_META_VALID);
}

void boot_meta_kill(void)
{
    /* F1 cho phép ghi 0x0000 đè lên half-word đã ghi. */
    (void)boot_flash_program_u16(OTA_META_BASE + 8u, 0x0000u);
}

uint32_t boot_image_crc32(uint32_t size)
{
    return ota_crc32_final(ota_crc32_update(OTA_CRC32_INIT, boot_flash_ptr(OTA_APP_BASE), size));
}

bool boot_vector_ok(uint32_t size)
{
    if (size < 8u || size > OTA_APP_MAX_SIZE) return false;
    uint32_t msp   = rd32(OTA_APP_BASE);
    uint32_t reset = rd32(OTA_APP_BASE + 4u);
    uint32_t entry = reset & ~1u;
    if ((msp & 7u) != 0u || msp <= OTA_SRAM_BASE || msp > OTA_SRAM_END) return false;
    if ((reset & 1u) == 0u) return false;
    return entry >= OTA_APP_BASE && entry < OTA_APP_BASE + size;
}

bool boot_application_valid(void)
{
    boot_meta_t m;
    boot_meta_read(&m);
    if (m.valid != OTA_META_VALID) return false;
    if (m.size < 8u || m.size > OTA_APP_MAX_SIZE) return false;
    if (!boot_vector_ok(m.size)) return false;
    return boot_image_crc32(m.size) == m.image_crc32;
}
