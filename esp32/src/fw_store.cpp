/* Lưu + kiểm tra firmware STM32 trước khi nạp. Lê Trọng Mạnh. Xem include/fw_store.h. */
#include <Arduino.h>
#include <LittleFS.h>
#include "fw_store.h"
#include "stm32_memory_map.h"

static File        s_file;
static uint32_t    s_size;
static uint32_t    s_crc;
static uint8_t     s_head[8];       /* 8 byte đầu: MSP + Reset_Handler */
static const char *s_err = "";

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len)
{
    while (len--) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
    return crc;
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static bool fail(const char *msg)
{
    s_err = msg;
    fw_store_abort();
    return false;
}

bool fw_store_begin(void)
{
    if (s_file) s_file.close();
    s_file = LittleFS.open(FW_PATH_NEW, "w");
    s_size = 0;
    s_crc  = 0xFFFFFFFFUL;
    s_err  = "";
    if (!s_file) return fail("khong mo duoc file tam");
    return true;
}

bool fw_store_write(const uint8_t *data, size_t len)
{
    if (!s_file) return false;                       /* đã lỗi trước đó */
    if (s_size + len > STM32_APP_MAX_SIZE) return fail("file vuot qua vung app");

    for (size_t i = 0; i < len && s_size + i < sizeof(s_head); i++)
        s_head[s_size + i] = data[i];

    if (s_file.write(data, len) != len) return fail("ghi LittleFS loi (het cho?)");
    s_crc   = crc32_update(s_crc, data, len);
    s_size += len;
    return true;
}

bool fw_store_end(void)
{
    if (!s_file) return false;
    s_file.close();

    if (s_size < sizeof(s_head)) return fail("file qua nho");

    /* Ảnh phải build cho địa chỉ STM32_APP_START. Ảnh standalone 0x08000000 sẽ bị loại ở đây. */
    uint32_t msp   = rd32(&s_head[0]);
    uint32_t reset = rd32(&s_head[4]);
    /* Cùng quy tắc với bootloader (boot_validate.c) để lỗi bị bắt ngay khi upload. */
    if ((msp & 7UL) != 0 || msp <= STM32_SRAM_START || msp > STM32_SRAM_END)
        return fail("MSP khong hop le - khong phai file .bin STM32?");
    if ((reset & 1UL) == 0 || (reset & ~1UL) < STM32_APP_START || (reset & ~1UL) >= STM32_APP_START + s_size)
        return fail("Reset vector ngoai anh app - build sai dia chi (can 0x08002000)?");

    uint32_t crc = crc32_final(s_crc);

    /* Đổi tên: cur -> prev, new -> cur. Mất điện giữa chừng: vẫn còn ít nhất 1 bản. */
    if (LittleFS.exists(FW_PATH_CUR)) {
        LittleFS.remove(FW_PATH_PREV);
        LittleFS.rename(FW_PATH_CUR, FW_PATH_PREV);
    }
    if (!LittleFS.rename(FW_PATH_NEW, FW_PATH_CUR)) return fail("doi ten file loi");

    File m = LittleFS.open(FW_PATH_META, "w");
    if (m) {
        m.printf("%lu %08lX\n", (unsigned long)s_size, (unsigned long)crc);
        m.close();
    }
    Serial.printf("[fw] luu xong %lu B, crc %08lX\n", (unsigned long)s_size, (unsigned long)crc);
    return true;
}

void fw_store_abort(void)
{
    if (s_file) s_file.close();
    LittleFS.remove(FW_PATH_NEW);
}

const char *fw_store_last_error(void) { return s_err; }

FwInfo fw_store_info(void)
{
    FwInfo info = {};
    info.present  = LittleFS.exists(FW_PATH_CUR);
    info.has_prev = LittleFS.exists(FW_PATH_PREV);
    File m = LittleFS.open(FW_PATH_META, "r");
    if (info.present && m) {
        unsigned long size = 0, crc = 0;
        String line = m.readStringUntil('\n');
        sscanf(line.c_str(), "%lu %lx", &size, &crc);
        info.size = size;
        info.crc  = crc;
    }
    if (m) m.close();
    return info;
}
