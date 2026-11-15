#ifndef FW_STORE_H
#define FW_STORE_H
/* Lưu firmware STM32 trong LittleFS của ESP32.
 *   /fw_new.bin  đang nhận (tạm, xoá nếu lỗi)
 *   /fw.bin      bản đã kiểm tra, sẽ nạp xuống STM32
 *   /fw_prev.bin bản trước, dùng để rollback nếu app mới không báo version
 *   /fw.meta     "size crc" của /fw.bin
 * Chỉ một luồng (WebServer loop) gọi các hàm này. */
#include <stddef.h>
#include <stdint.h>

#define FW_PATH_NEW  "/fw_new.bin"
#define FW_PATH_CUR  "/fw.bin"
#define FW_PATH_PREV "/fw_prev.bin"
#define FW_PATH_META "/fw.meta"

/* CRC-32 IEEE (giống zlib crc32): poly 0x04C11DB7 reflected (0xEDB88320),
 * init 0xFFFFFFFF, xorout 0xFFFFFFFF. Test vector: "123456789" -> 0xCBF43926.
 * Bootloader STM32 phải dùng đúng thuật toán này (bản phần mềm, không dùng CRC unit). */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len);  /* crc bắt đầu = 0xFFFFFFFF */
static inline uint32_t crc32_final(uint32_t crc) { return crc ^ 0xFFFFFFFFUL; }

/* Nhận file theo từng đoạn. begin -> write... -> end (hoặc abort). */
bool fw_store_begin(void);
bool fw_store_write(const uint8_t *data, size_t len);
bool fw_store_end(void);           /* kiểm tra ảnh + đổi tên vào /fw.bin */
void fw_store_abort(void);
const char *fw_store_last_error(void);

struct FwInfo {
    bool     present;
    uint32_t size;
    uint32_t crc;
    bool     has_prev;
};
FwInfo fw_store_info(void);

#endif
