#ifndef OTA_FRAME_H
#define OTA_FRAME_H
/* Codec frame OTA UART v1 — dùng chung cho ESP32, app STM32 và bootloader.
 * Đặc tả: docs/OTA_CONTRACT.md.
 *   [A5 5A] [LEN u8] [BODY: TYPE + trường, LEN byte] [CRC16 u16 LE trên LEN+BODY]
 * File này thuần C, không phụ thuộc phần cứng, không malloc. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTA_MAGIC0          0xA5u
#define OTA_MAGIC1          0x5Au
#define OTA_LEN_MIN         1u
#define OTA_LEN_MAX         131u                  /* TYPE + seq(2) + 128 byte */
#define OTA_FRAME_MAX       (2u + 1u + OTA_LEN_MAX + 2u)   /* 136 */
#define OTA_DATA_CHUNK      128u
#define OTA_INTER_BYTE_MS   20u

/* TYPE */
#define OTA_T_ENTER_UPDATE  0x01u
#define OTA_T_READY         0x02u
#define OTA_T_BEGIN         0x10u
#define OTA_T_DATA          0x11u
#define OTA_T_END           0x12u
#define OTA_T_ACK           0x70u
#define OTA_T_NACK          0x71u

/* Mã lỗi trong NACK */
#define OTA_E_BAD_STATE        0x01u
#define OTA_E_BAD_LENGTH       0x02u
#define OTA_E_BAD_IMAGE_SIZE   0x03u
#define OTA_E_BAD_SEQ          0x04u
#define OTA_E_FLASH_ERROR      0x05u
#define OTA_E_INCOMPLETE       0x06u
#define OTA_E_IMAGE_CRC_ERROR  0x07u
#define OTA_E_BAD_VECTOR       0x08u
#define OTA_E_UNSUPPORTED_TYPE 0x09u

#define OTA_SEQ_NONE        0xFFFFu

/* ---- CRC ---- */
/* CRC-16/IBM-3740 (CCITT-FALSE): poly 0x1021, init 0xFFFF, không reflect, xorout 0.
 * "123456789" -> 0x29B1. */
uint16_t ota_crc16(const uint8_t *data, size_t len);
/* CRC-32/ISO-HDLC (zlib): bắt đầu crc = OTA_CRC32_INIT, kết thúc ota_crc32_final().
 * "123456789" -> 0xCBF43926. */
#define OTA_CRC32_INIT 0xFFFFFFFFu
uint32_t ota_crc32_update(uint32_t crc, const uint8_t *data, size_t len);
static inline uint32_t ota_crc32_final(uint32_t crc) { return crc ^ 0xFFFFFFFFu; }

/* ---- Little-endian ---- */
static inline void ota_put_u16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static inline void ota_put_u32(uint8_t *p, uint32_t v)
{ p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
static inline uint16_t ota_get_u16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline uint32_t ota_get_u32(const uint8_t *p)
{ return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

/* ---- Encoder ----
 * Ghi frame hoàn chỉnh vào out (cần >= len + 5 byte). Trả số byte, 0 nếu len sai. */
size_t ota_encode(uint8_t *out, const uint8_t *body, uint8_t len);

size_t ota_build_enter_update(uint8_t *out);
size_t ota_build_ready(uint8_t *out);
size_t ota_build_begin(uint8_t *out, uint32_t size, uint32_t image_crc32);
size_t ota_build_data(uint8_t *out, uint16_t seq, const uint8_t *bytes, uint8_t n);  /* n = 1..128 */
size_t ota_build_end(uint8_t *out);
size_t ota_build_ack(uint8_t *out, uint8_t acked_type);                 /* LEN 2 */
size_t ota_build_ack_data(uint8_t *out, uint16_t seq);                  /* LEN 4 */
size_t ota_build_nack(uint8_t *out, uint8_t rejected_type, uint8_t error, uint16_t seq);

/* ---- Parser (máy trạng thái theo từng byte) ---- */
typedef enum {
    OTA_RX_MAGIC0 = 0,
    OTA_RX_MAGIC1,
    OTA_RX_LEN,
    OTA_RX_BODY,
    OTA_RX_CRC0,
    OTA_RX_CRC1
} ota_rx_state_t;

typedef struct {
    ota_rx_state_t state;
    uint8_t  len;
    uint8_t  idx;
    uint8_t  crc_lo;
    uint32_t last_ms;
    uint8_t  body[OTA_LEN_MAX];      /* body[0] = TYPE */
} ota_parser_t;

void ota_parser_reset(ota_parser_t *p);
/* Đưa 1 byte vào. now_ms dùng cho timeout 20 ms giữa các byte.
 * Trả true khi vừa nhận xong một frame đúng CRC: p->body / p->len hợp lệ tới lần gọi sau.
 * Frame sai (LEN ngoài giới hạn, CRC sai, quá hạn) bị bỏ im lặng, parser quay lại dò magic. */
bool ota_parser_feed(ota_parser_t *p, uint8_t byte, uint32_t now_ms);
/* Gọi khi phần cứng UART báo lỗi (overrun/framing/noise): bỏ frame đang nhận. */
static inline void ota_parser_on_uart_error(ota_parser_t *p) { ota_parser_reset(p); }

const char *ota_error_name(uint8_t error);

#ifdef __cplusplus
}
#endif
#endif
