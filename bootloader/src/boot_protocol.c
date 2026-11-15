/* Máy trạng thái bootloader OTA v1 (docs/OTA_CONTRACT.md mục 5). Lê Trọng Mạnh.
 * Thuần logic trên boot_port.h / boot_flash.h / boot_validate.h, chạy được trong test PC. */
#include "boot_protocol.h"
#include "boot_flash.h"
#include "boot_port.h"
#include "boot_validate.h"
#include "ota_frame.h"
#include "ota_layout.h"
#include <string.h>

static struct {
    boot_state_t state;
    ota_parser_t rx;
    uint32_t size;          /* phiên hiện tại */
    uint32_t image_crc32;
    uint32_t written;       /* số byte file đã ghi */
    uint16_t next_seq;
    uint32_t t_progress;    /* ACK gần nhất của BEGIN/DATA */
    uint32_t t_grace;       /* sau ACK(END) đầu tiên */
    uint8_t  tx[16];
} s;

static void send(size_t n) { port_uart_write(s.tx, n); }
static void send_ready(void) { send(ota_build_ready(s.tx)); }
static void send_ack(uint8_t type) { send(ota_build_ack(s.tx, type)); }
static void send_ack_data(uint16_t seq) { send(ota_build_ack_data(s.tx, seq)); }
static void send_nack(uint8_t type, uint8_t err, uint16_t seq) { send(ota_build_nack(s.tx, type, err, seq)); }

static void to_wait_begin(void)
{
    s.state = BOOT_ST_WAIT_BEGIN;
    s.size = s.image_crc32 = s.written = 0u;
    s.next_seq = 0u;
}

void boot_protocol_init(void)
{
    ota_parser_reset(&s.rx);
    to_wait_begin();
    send_ready();
}

boot_state_t boot_protocol_state(void) { return s.state; }

/* ---------------- ENTER_UPDATE ---------------- */
static void on_enter_update(uint8_t len)
{
    if (len != 1u) { send_nack(OTA_T_ENTER_UPDATE, OTA_E_BAD_LENGTH, OTA_SEQ_NONE); return; }
    switch (s.state) {
    case BOOT_ST_WAIT_BEGIN:
        send_ready();                          /* READY bị mất: trả thêm READY, không xoá Flash */
        break;
    case BOOT_ST_RECEIVING:
        /* ESP32 đã bỏ phiên cũ và mở phiên mới. VALID vẫn thiếu nên app không chạy được;
         * huỷ phiên RAM, trả READY để ESP32 gửi BEGIN lại (không phải chờ 15 s). */
        to_wait_begin();
        send_ready();
        break;
    default:
        send_nack(OTA_T_ENTER_UPDATE, OTA_E_BAD_STATE, OTA_SEQ_NONE);
        break;
    }
}

/* ---------------- BEGIN ---------------- */
static void on_begin(const uint8_t *b, uint8_t len, uint32_t now)
{
    if (len != 9u) { send_nack(OTA_T_BEGIN, OTA_E_BAD_LENGTH, OTA_SEQ_NONE); return; }
    uint32_t size = ota_get_u32(&b[1]);
    uint32_t crc  = ota_get_u32(&b[5]);

    if (s.state == BOOT_ST_RECEIVING) {
        if (s.next_seq == 0u && size == s.size && crc == s.image_crc32) {
            send_ack(OTA_T_BEGIN);             /* ACK(BEGIN) bị mất: ACK lại, không xoá thêm */
            s.t_progress = now;
        } else {
            send_nack(OTA_T_BEGIN, OTA_E_BAD_STATE, OTA_SEQ_NONE);
        }
        return;
    }
    if (s.state != BOOT_ST_WAIT_BEGIN) { send_nack(OTA_T_BEGIN, OTA_E_BAD_STATE, OTA_SEQ_NONE); return; }

    /* Kiểm tra trước, chưa đụng Flash: app/metadata cũ giữ nguyên nếu size sai. */
    if (size < 8u || size > OTA_APP_MAX_SIZE) { send_nack(OTA_T_BEGIN, OTA_E_BAD_IMAGE_SIZE, OTA_SEQ_NONE); return; }

    /* Vô hiệu metadata TRƯỚC, rồi mới xoá app. */
    bool ok = boot_meta_invalidate();
    uint32_t pages = (size + OTA_PAGE_SIZE - 1u) / OTA_PAGE_SIZE;
    for (uint32_t i = 0; ok && i < pages; i++)
        ok = boot_flash_erase_page(OTA_APP_BASE + i * OTA_PAGE_SIZE);
    if (!ok) {
        to_wait_begin();
        send_nack(OTA_T_BEGIN, OTA_E_FLASH_ERROR, OTA_SEQ_NONE);
        return;
    }

    s.state = BOOT_ST_RECEIVING;
    s.size = size;
    s.image_crc32 = crc;
    s.written = 0u;
    s.next_seq = 0u;
    send_ack(OTA_T_BEGIN);
    s.t_progress = now;
}

/* ---------------- DATA ---------------- */
static void on_data(const uint8_t *b, uint8_t len, uint32_t now)
{
    uint16_t seq = (len >= 3u) ? ota_get_u16(&b[1]) : OTA_SEQ_NONE;
    if (len < 4u) { send_nack(OTA_T_DATA, OTA_E_BAD_LENGTH, seq); return; }
    if (s.state != BOOT_ST_RECEIVING) { send_nack(OTA_T_DATA, OTA_E_BAD_STATE, seq); return; }

    const uint8_t *payload = &b[3];
    uint32_t n = (uint32_t)len - 3u;

    if (seq == s.next_seq) {
        if (s.written >= s.size) { send_nack(OTA_T_DATA, OTA_E_BAD_SEQ, seq); return; }   /* thừa gói */
        uint32_t remain   = s.size - s.written;
        uint32_t expected = remain < OTA_DATA_CHUNK ? remain : OTA_DATA_CHUNK;
        uint32_t offset   = (uint32_t)seq * OTA_DATA_CHUNK;
        uint32_t addr     = OTA_APP_BASE + offset;
        if (n != expected) { send_nack(OTA_T_DATA, OTA_E_BAD_LENGTH, seq); return; }
        if (offset != s.written) { send_nack(OTA_T_DATA, OTA_E_BAD_SEQ, seq); return; }
        if (addr + ((n + 1u) & ~1u) > OTA_APP_LIMIT) { send_nack(OTA_T_DATA, OTA_E_BAD_LENGTH, seq); return; }

        if (!boot_flash_program(addr, payload, n)) {
            /* Không ghi lại cùng half-word trong phiên cũ: bắt đầu lại từ BEGIN. */
            to_wait_begin();
            send_nack(OTA_T_DATA, OTA_E_FLASH_ERROR, seq);
            return;
        }
        s.written += n;
        s.next_seq++;
        send_ack_data(seq);
        s.t_progress = now;
        return;
    }

    if (s.next_seq > 0u && seq == (uint16_t)(s.next_seq - 1u)) {
        /* Gói trùng (ACK bị mất): so với khối vừa ghi, đúng thì chỉ ACK lại. */
        uint32_t offset = (uint32_t)seq * OTA_DATA_CHUNK;
        uint32_t prev_n = s.written - offset;
        if (n == prev_n && memcmp(boot_flash_ptr(OTA_APP_BASE + offset), payload, n) == 0) {
            send_ack_data(seq);
            s.t_progress = now;
        } else {
            send_nack(OTA_T_DATA, OTA_E_BAD_SEQ, seq);
        }
        return;
    }

    send_nack(OTA_T_DATA, OTA_E_BAD_SEQ, seq);
}

/* ---------------- END ---------------- */
static void on_end(uint8_t len, uint32_t now)
{
    (void)now;
    if (len != 1u) { send_nack(OTA_T_END, OTA_E_BAD_LENGTH, OTA_SEQ_NONE); return; }

    if (s.state == BOOT_ST_COMPLETE_GRACE) { send_ack(OTA_T_END); return; }   /* không gia hạn cửa sổ */
    if (s.state != BOOT_ST_RECEIVING) { send_nack(OTA_T_END, OTA_E_BAD_STATE, OTA_SEQ_NONE); return; }
    if (s.written < s.size) { send_nack(OTA_T_END, OTA_E_INCOMPLETE, OTA_SEQ_NONE); return; }  /* giữ tiến độ */

    if (boot_image_crc32(s.size) != s.image_crc32) {
        to_wait_begin();
        send_nack(OTA_T_END, OTA_E_IMAGE_CRC_ERROR, OTA_SEQ_NONE);
        return;
    }
    if (!boot_vector_ok(s.size)) {
        to_wait_begin();
        send_nack(OTA_T_END, OTA_E_BAD_VECTOR, OTA_SEQ_NONE);
        return;
    }
    if (!boot_meta_commit(s.size, s.image_crc32)) {
        boot_meta_kill();
        to_wait_begin();
        send_nack(OTA_T_END, OTA_E_FLASH_ERROR, OTA_SEQ_NONE);
        return;
    }
    send_ack(OTA_T_END);                       /* trả về khi byte cuối đã ra dây (TC) */
    s.state = BOOT_ST_COMPLETE_GRACE;
    s.t_grace = port_ms();
}

/* ---------------- Phân loại frame ---------------- */
static void on_frame(const uint8_t *b, uint8_t len, uint32_t now)
{
    switch (b[0]) {
    case OTA_T_ENTER_UPDATE: on_enter_update(len);   break;
    case OTA_T_BEGIN:        on_begin(b, len, now);  break;
    case OTA_T_DATA:         on_data(b, len, now);   break;
    case OTA_T_END:          on_end(len, now);       break;
    case OTA_T_READY:
    case OTA_T_ACK:
    case OTA_T_NACK:         /* có trong v1 nhưng sai chiều */
        send_nack(b[0], OTA_E_BAD_STATE, OTA_SEQ_NONE);
        break;
    default:
        send_nack(b[0], OTA_E_UNSUPPORTED_TYPE, OTA_SEQ_NONE);
        break;
    }
}

void boot_protocol_poll(void)
{
    uint8_t byte;
    int r;
    while ((r = port_uart_getc(&byte)) != 0) {
        if (r < 0) { ota_parser_on_uart_error(&s.rx); continue; }
        if (ota_parser_feed(&s.rx, byte, port_ms()))
            on_frame(s.rx.body, s.rx.len, port_ms());
    }

    uint32_t now = port_ms();
    if (s.state == BOOT_ST_RECEIVING && (uint32_t)(now - s.t_progress) > BOOT_SESSION_TIMEOUT_MS)
        to_wait_begin();                       /* VALID vẫn thiếu; ESP32 mở phiên mới */
    if (s.state == BOOT_ST_COMPLETE_GRACE && (uint32_t)(now - s.t_grace) >= BOOT_GRACE_MS)
        s.state = BOOT_ST_RUN_APP;
}
