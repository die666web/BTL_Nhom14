/* Codec frame OTA UART v1. Xem ota_frame.h và docs/OTA_CONTRACT.md. */
#include "ota_frame.h"
#include <string.h>

static uint16_t crc16_update(uint16_t crc, const uint8_t *data, size_t len)
{
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (int i = 0; i < 8; i++)
            crc = (crc & 0x8000u) ? (uint16_t)((crc << 1) ^ 0x1021u) : (uint16_t)(crc << 1);
    }
    return crc;
}

uint16_t ota_crc16(const uint8_t *data, size_t len) { return crc16_update(0xFFFFu, data, len); }

uint32_t ota_crc32_update(uint32_t crc, const uint8_t *data, size_t len)
{
    while (len--) {
        crc ^= *data++;
        for (int i = 0; i < 8; i++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc;
}

size_t ota_encode(uint8_t *out, const uint8_t *body, uint8_t len)
{
    if (len < OTA_LEN_MIN || len > OTA_LEN_MAX) return 0;
    out[0] = OTA_MAGIC0;
    out[1] = OTA_MAGIC1;
    out[2] = len;
    if (body != &out[3]) memmove(&out[3], body, len);
    ota_put_u16(&out[3 + len], ota_crc16(&out[2], (size_t)len + 1u));
    return (size_t)len + 5u;
}

static size_t build1(uint8_t *out, uint8_t type)
{
    out[3] = type;
    return ota_encode(out, &out[3], 1);
}

size_t ota_build_enter_update(uint8_t *out) { return build1(out, OTA_T_ENTER_UPDATE); }
size_t ota_build_ready(uint8_t *out)        { return build1(out, OTA_T_READY); }
size_t ota_build_end(uint8_t *out)          { return build1(out, OTA_T_END); }

size_t ota_build_begin(uint8_t *out, uint32_t size, uint32_t image_crc32)
{
    uint8_t *b = &out[3];
    b[0] = OTA_T_BEGIN;
    ota_put_u32(&b[1], size);
    ota_put_u32(&b[5], image_crc32);
    return ota_encode(out, b, 9);
}

size_t ota_build_data(uint8_t *out, uint16_t seq, const uint8_t *bytes, uint8_t n)
{
    if (n == 0 || n > OTA_DATA_CHUNK) return 0;
    uint8_t *b = &out[3];
    b[0] = OTA_T_DATA;
    ota_put_u16(&b[1], seq);
    memmove(&b[3], bytes, n);
    return ota_encode(out, b, (uint8_t)(n + 3u));
}

size_t ota_build_ack(uint8_t *out, uint8_t acked_type)
{
    uint8_t *b = &out[3];
    b[0] = OTA_T_ACK;
    b[1] = acked_type;
    return ota_encode(out, b, 2);
}

size_t ota_build_ack_data(uint8_t *out, uint16_t seq)
{
    uint8_t *b = &out[3];
    b[0] = OTA_T_ACK;
    b[1] = OTA_T_DATA;
    ota_put_u16(&b[2], seq);
    return ota_encode(out, b, 4);
}

size_t ota_build_nack(uint8_t *out, uint8_t rejected_type, uint8_t error, uint16_t seq)
{
    uint8_t *b = &out[3];
    b[0] = OTA_T_NACK;
    b[1] = rejected_type;
    b[2] = error;
    ota_put_u16(&b[3], seq);
    return ota_encode(out, b, 5);
}

void ota_parser_reset(ota_parser_t *p)
{
    p->state = OTA_RX_MAGIC0;
    p->len = 0;
    p->idx = 0;
}

bool ota_parser_feed(ota_parser_t *p, uint8_t byte, uint32_t now_ms)
{
    /* Đang giữa frame mà im quá 20 ms: bỏ frame, xét byte này như byte mới. */
    if (p->state != OTA_RX_MAGIC0 && (uint32_t)(now_ms - p->last_ms) > OTA_INTER_BYTE_MS)
        ota_parser_reset(p);
    p->last_ms = now_ms;

    switch (p->state) {
    case OTA_RX_MAGIC0:
        if (byte == OTA_MAGIC0) p->state = OTA_RX_MAGIC1;
        break;
    case OTA_RX_MAGIC1:
        if (byte == OTA_MAGIC1) p->state = OTA_RX_LEN;
        else if (byte != OTA_MAGIC0) p->state = OTA_RX_MAGIC0;   /* A5 A5 5A vẫn bắt được */
        break;
    case OTA_RX_LEN:
        if (byte < OTA_LEN_MIN || byte > OTA_LEN_MAX) {           /* kiểm tra trước khi nhận BODY */
            ota_parser_reset(p);
        } else {
            p->len = byte;
            p->idx = 0;
            p->state = OTA_RX_BODY;
        }
        break;
    case OTA_RX_BODY:                                            /* A5 5A ở đây là dữ liệu bình thường */
        p->body[p->idx++] = byte;
        if (p->idx == p->len) p->state = OTA_RX_CRC0;
        break;
    case OTA_RX_CRC0:
        p->crc_lo = byte;
        p->state = OTA_RX_CRC1;
        break;
    case OTA_RX_CRC1: {
        uint16_t got = (uint16_t)(p->crc_lo | (byte << 8));
        uint16_t crc = crc16_update(0xFFFFu, &p->len, 1);         /* CRC trên LEN + BODY */
        crc = crc16_update(crc, p->body, p->len);
        p->state = OTA_RX_MAGIC0;
        return crc == got;                                       /* sai CRC: im lặng */
    }
    }
    return false;
}

const char *ota_error_name(uint8_t error)
{
    switch (error) {
    case OTA_E_BAD_STATE:        return "BAD_STATE";
    case OTA_E_BAD_LENGTH:       return "BAD_LENGTH";
    case OTA_E_BAD_IMAGE_SIZE:   return "BAD_IMAGE_SIZE";
    case OTA_E_BAD_SEQ:          return "BAD_SEQ";
    case OTA_E_FLASH_ERROR:      return "FLASH_ERROR";
    case OTA_E_INCOMPLETE:       return "INCOMPLETE";
    case OTA_E_IMAGE_CRC_ERROR:  return "IMAGE_CRC_ERROR";
    case OTA_E_BAD_VECTOR:       return "BAD_VECTOR";
    case OTA_E_UNSUPPORTED_TYPE: return "UNSUPPORTED_TYPE";
    default:                     return "UNKNOWN";
    }
}
