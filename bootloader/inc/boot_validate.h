#ifndef BOOT_VALIDATE_H
#define BOOT_VALIDATE_H
/* Metadata + kiểm tra ảnh app. CRC chỉ phát hiện lỗi, không phải xác thực chữ ký. */
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t size;
    uint32_t image_crc32;
    uint16_t valid;
} boot_meta_t;

void     boot_meta_read(boot_meta_t *meta);
/* Xoá page metadata và kiểm tra marker đã về 0xFFFF. Phải gọi TRƯỚC khi xoá vùng app. */
bool     boot_meta_invalidate(void);
/* Ghi + đọc lại size, crc; cuối cùng ghi + đọc lại valid = 0xA55A. */
bool     boot_meta_commit(uint32_t size, uint32_t image_crc32);
/* Cố vô hiệu marker (ghi 0x0000) sau khi commit lỗi. */
void     boot_meta_kill(void);

uint32_t boot_image_crc32(uint32_t size);
/* MSP căn 8, trong (SRAM_BASE, SRAM_END]; reset vector có bit Thumb, nằm trong ảnh. */
bool     boot_vector_ok(uint32_t size);
/* Marker đúng, size trong giới hạn, vector hợp lệ, CRC-32 Flash khớp metadata. */
bool     boot_application_valid(void);

#endif
