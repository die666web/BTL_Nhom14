/* Test PC cho OTA v1: codec frame (common/ota_frame) + máy trạng thái bootloader
 * (bootloader/src/boot_protocol.c, boot_validate.c) chạy trên Flash/UART giả lập.
 * Chạy: sh tests/run_ota_tests.sh   (cần gcc/clang trên máy tính)
 * Không mô phỏng thời gian xoá/ghi Flash thật, ngắt hay lỗi điện. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "ota_frame.h"
#include "ota_layout.h"
#include "boot_flash.h"
#include "boot_port.h"
#include "boot_protocol.h"
#include "boot_validate.h"

static unsigned checks;
#define CHECK(x) do { if (!(x)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #x); assert(0); } ++checks; } while (0)

/* ---------------- Flash giả lập ---------------- */
static uint8_t  flash[OTA_FLASH_END - OTA_FLASH_BASE];
static unsigned erase_count, valid_commits;
#define FL(a) (&flash[(a) - OTA_FLASH_BASE])

const uint8_t *boot_flash_ptr(uint32_t a) { return FL(a); }

bool boot_flash_erase_page(uint32_t a)
{
    if (a % OTA_PAGE_SIZE || a < OTA_APP_BASE || a >= OTA_FLASH_END) return false;
    memset(FL(a), 0xFF, OTA_PAGE_SIZE);
    erase_count++;
    return true;
}

bool boot_flash_program_u16(uint32_t a, uint16_t v)
{
    if ((a & 1u) || a < OTA_APP_BASE || a + 2u > OTA_FLASH_END) return false;
    uint16_t cur = (uint16_t)(FL(a)[0] | (FL(a)[1] << 8));
    if (cur != 0xFFFFu && v != 0u) return false;          /* như F1: chỉ ghi lên ô đã xoá, trừ 0x0000 */
    FL(a)[0] = (uint8_t)v;
    FL(a)[1] = (uint8_t)(v >> 8);
    if (a == OTA_META_BASE + 8u && v == OTA_META_VALID) valid_commits++;
    return true;
}

bool boot_flash_program(uint32_t a, const uint8_t *d, size_t n)
{
    for (size_t i = 0; i < n; i += 2u) {
        uint16_t hw = (uint16_t)(d[i] | ((i + 1u < n ? d[i + 1u] : 0xFFu) << 8));
        if (!boot_flash_program_u16(a + (uint32_t)i, hw)) return false;
    }
    return true;
}

/* ---------------- UART / thời gian giả lập ---------------- */
static uint32_t now;
static uint8_t  rxq[512]; static size_t rx_head, rx_tail;   /* ESP32 -> bootloader */
static uint8_t  txq[512]; static size_t tx_len;             /* bootloader -> ESP32 */

bool     port_take_update_flag(void) { return false; }
void     port_init(void) { }
uint32_t port_ms(void) { return now; }
void     port_led(bool on) { (void)on; }
void     port_deinit(void) { }
int port_uart_getc(uint8_t *b)
{
    if (rx_head == rx_tail) return 0;
    *b = rxq[rx_head++];
    return 1;
}
void port_uart_write(const uint8_t *d, size_t n)
{
    assert(tx_len + n <= sizeof txq);
    memcpy(&txq[tx_len], d, n);
    tx_len += n;
}

/* Gửi frame tới bootloader rồi chạy một vòng poll. */
static void xfer(const uint8_t *f, size_t n)
{
    rx_head = rx_tail = 0;
    memcpy(rxq, f, n);
    rx_tail = n;
    boot_protocol_poll();
}

/* Đọc các frame bootloader đã gửi, giữ lại frame cuối. Trả số frame. */
static uint8_t resp[OTA_LEN_MAX], resp_len;
static int responses(void)
{
    ota_parser_t p;
    ota_parser_reset(&p);
    int n = 0;
    for (size_t i = 0; i < tx_len; i++)
        if (ota_parser_feed(&p, txq[i], now)) { memcpy(resp, p.body, p.len); resp_len = p.len; n++; }
    tx_len = 0;
    return n;
}
static void expect_ready(void) { CHECK(responses() == 1); CHECK(resp_len == 1 && resp[0] == OTA_T_READY); }
static void expect_none(void)  { CHECK(responses() == 0); }
static void expect_ack(uint8_t t)
{ CHECK(responses() == 1); CHECK(resp_len == 2 && resp[0] == OTA_T_ACK && resp[1] == t); }
static void expect_ack_data(uint16_t seq)
{ CHECK(responses() == 1); CHECK(resp_len == 4 && resp[0] == OTA_T_ACK && resp[1] == OTA_T_DATA && ota_get_u16(&resp[2]) == seq); }
static void expect_nack(uint8_t t, uint8_t err)
{ CHECK(responses() == 1); CHECK(resp_len == 5 && resp[0] == OTA_T_NACK && resp[1] == t && resp[2] == err); }

/* ---------------- Ảnh app giả ---------------- */
static uint8_t img[OTA_APP_MAX_SIZE];
static uint32_t make_image(uint32_t size, uint32_t msp)
{
    for (uint32_t i = 0; i < size; i++) img[i] = (uint8_t)(i * 7u + 3u);
    ota_put_u32(&img[0], msp);
    ota_put_u32(&img[4], OTA_APP_BASE + 9u);             /* Thumb, nằm trong ảnh */
    return ota_crc32_final(ota_crc32_update(OTA_CRC32_INIT, img, size));
}

static uint8_t f[OTA_FRAME_MAX];
static void send_begin(uint32_t size, uint32_t crc) { xfer(f, ota_build_begin(f, size, crc)); }
static void send_data(uint16_t seq, uint32_t size)
{
    uint32_t off = (uint32_t)seq * OTA_DATA_CHUNK;
    uint32_t n = size - off < OTA_DATA_CHUNK ? size - off : OTA_DATA_CHUNK;
    xfer(f, ota_build_data(f, seq, &img[off], (uint8_t)n));
}
static void send_end(void) { xfer(f, ota_build_end(f)); }

static void fresh(void)
{
    memset(flash, 0xFF, sizeof flash);
    erase_count = valid_commits = 0;
    now = 1000;
    tx_len = 0;
    boot_protocol_init();
    expect_ready();
}

/* Phiên đầy đủ, trả về sau khi bootloader sẵn sàng chạy app. */
static void full_session(uint32_t size, uint32_t crc)
{
    send_begin(size, crc);
    expect_ack(OTA_T_BEGIN);
    uint16_t packets = (uint16_t)((size + OTA_DATA_CHUNK - 1u) / OTA_DATA_CHUNK);
    for (uint16_t s = 0; s < packets; s++) { send_data(s, size); expect_ack_data(s); }
    send_end();
    expect_ack(OTA_T_END);
    CHECK(boot_protocol_state() == BOOT_ST_COMPLETE_GRACE);
    now += BOOT_GRACE_MS;
    boot_protocol_poll();
    CHECK(boot_protocol_state() == BOOT_ST_RUN_APP);
    CHECK(boot_application_valid());
}

/* ================= Codec ================= */
static void test_codec(void)
{
    static const uint8_t ascii[] = "123456789";
    CHECK(ota_crc16(ascii, 9) == 0x29B1u);
    CHECK(ota_crc32_final(ota_crc32_update(OTA_CRC32_INIT, ascii, 9)) == 0xCBF43926u);

    static const struct { const char *name; uint8_t bytes[16]; size_t n; } v[] = {
        { "ENTER", {0xA5,0x5A,0x01,0x01,0x1F,0x3E}, 6 },
        { "READY", {0xA5,0x5A,0x01,0x02,0x7C,0x0E}, 6 },
        { "BEGIN", {0xA5,0x5A,0x09,0x10,0xE8,0x03,0x00,0x00,0x26,0x39,0xF4,0xCB,0x69,0xDD}, 14 },
        { "DATA",  {0xA5,0x5A,0x06,0x11,0x05,0x00,0x11,0x22,0x33,0x7F,0x3E}, 11 },
        { "ACKD",  {0xA5,0x5A,0x04,0x70,0x11,0x05,0x00,0xD9,0x51}, 9 },
        { "END",   {0xA5,0x5A,0x01,0x12,0x4D,0x1C}, 6 },
        { "ACKE",  {0xA5,0x5A,0x02,0x70,0x12,0xD6,0x98}, 7 },
        { "NACK",  {0xA5,0x5A,0x05,0x71,0x10,0x03,0xFF,0xFF,0x3E,0xA5}, 10 },
    };
    uint8_t out[OTA_FRAME_MAX];
    static const uint8_t d3[] = {0x11, 0x22, 0x33};
    size_t n[8];
    n[0] = ota_build_enter_update(out); CHECK(n[0] == v[0].n && !memcmp(out, v[0].bytes, n[0]));
    n[1] = ota_build_ready(out);        CHECK(n[1] == v[1].n && !memcmp(out, v[1].bytes, n[1]));
    n[2] = ota_build_begin(out, 1000, 0xCBF43926u); CHECK(n[2] == v[2].n && !memcmp(out, v[2].bytes, n[2]));
    n[3] = ota_build_data(out, 5, d3, 3);           CHECK(n[3] == v[3].n && !memcmp(out, v[3].bytes, n[3]));
    n[4] = ota_build_ack_data(out, 5);              CHECK(n[4] == v[4].n && !memcmp(out, v[4].bytes, n[4]));
    n[5] = ota_build_end(out);                      CHECK(n[5] == v[5].n && !memcmp(out, v[5].bytes, n[5]));
    n[6] = ota_build_ack(out, OTA_T_END);           CHECK(n[6] == v[6].n && !memcmp(out, v[6].bytes, n[6]));
    n[7] = ota_build_nack(out, OTA_T_BEGIN, OTA_E_BAD_IMAGE_SIZE, OTA_SEQ_NONE);
    CHECK(n[7] == v[7].n && !memcmp(out, v[7].bytes, n[7]));

    ota_parser_t p;
    for (size_t k = 0; k < 8; k++) {                     /* decoder đọc đúng mọi frame mẫu */
        ota_parser_reset(&p);
        int frames = 0;
        for (size_t i = 0; i < v[k].n; i++) frames += ota_parser_feed(&p, v[k].bytes[i], 0);
        CHECK(frames == 1 && p.len == v[k].bytes[2] && p.body[0] == v[k].bytes[3]);
    }

    /* Magic trong payload vẫn là một frame */
    static const uint8_t magic[] = {0xA5, 0x5A, 0xA5, 0x5A};
    size_t m = ota_build_data(out, 0, magic, 4);
    ota_parser_reset(&p);
    int frames = 0;
    for (size_t i = 0; i < m; i++) frames += ota_parser_feed(&p, out[i], 0);
    CHECK(frames == 1 && p.len == 7 && !memcmp(&p.body[3], magic, 4));

    /* Sai CRC: im lặng; frame đúng phía sau vẫn đọc được */
    uint8_t bad[6]; memcpy(bad, v[0].bytes, 6); bad[5] ^= 1;
    ota_parser_reset(&p);
    frames = 0;
    for (size_t i = 0; i < 6; i++) frames += ota_parser_feed(&p, bad[i], 0);
    for (size_t i = 0; i < 6; i++) frames += ota_parser_feed(&p, v[0].bytes[i], 0);
    CHECK(frames == 1);

    /* LEN ngoài giới hạn: bỏ ngay */
    static const uint8_t badlen[] = {0xA5, 0x5A, 0x00, 0xA5, 0x5A, 0x84};
    ota_parser_reset(&p);
    frames = 0;
    for (size_t i = 0; i < sizeof badlen; i++) frames += ota_parser_feed(&p, badlen[i], 0);
    CHECK(frames == 0 && p.state == OTA_RX_MAGIC0);

    /* Mất byte + quá 20 ms giữa các byte: bỏ frame dở, frame sau vẫn đọc được */
    ota_parser_reset(&p);
    frames = 0;
    for (size_t i = 0; i < 4; i++) frames += ota_parser_feed(&p, v[2].bytes[i], 100);
    for (size_t i = 0; i < 6; i++) frames += ota_parser_feed(&p, v[0].bytes[i], 200);
    CHECK(frames == 1 && p.body[0] == OTA_T_ENTER_UPDATE);
}

/* ================= Bootloader ================= */
static void test_sizes(void)
{
    uint32_t crc = make_image(1000, OTA_SRAM_END);       /* gói cuối 104 B */
    fresh(); full_session(1000, crc);

    crc = make_image(1001, OTA_SRAM_END);                /* gói cuối 105 B, đệm FF */
    fresh(); full_session(1001, crc);
    CHECK(*FL(OTA_APP_BASE + 1000u) == img[1000] && *FL(OTA_APP_BASE + 1001u) == 0xFF);

    crc = make_image(OTA_APP_MAX_SIZE, OTA_SRAM_END);    /* ảnh lớn nhất */
    fresh(); full_session(OTA_APP_MAX_SIZE, crc);
}

static void test_lost_ready_and_begin_ack(void)
{
    uint32_t crc = make_image(1000, OTA_SRAM_END);
    fresh();
    xfer(f, ota_build_enter_update(f)); expect_ready(); /* READY mất -> ENTER gửi lại */
    send_begin(1000, crc); expect_ack(OTA_T_BEGIN);
    unsigned erased = erase_count;
    send_begin(1000, crc); expect_ack(OTA_T_BEGIN);     /* ACK(BEGIN) mất */
    CHECK(erase_count == erased);                       /* không xoá thêm */
    send_begin(2000, crc); expect_nack(OTA_T_BEGIN, OTA_E_BAD_STATE);
}

static void test_duplicate_and_bad_data(void)
{
    uint32_t crc = make_image(1000, OTA_SRAM_END);
    fresh();
    send_begin(1000, crc); expect_ack(OTA_T_BEGIN);
    send_data(0, 1000); expect_ack_data(0);
    send_data(0, 1000); expect_ack_data(0);             /* ACK(DATA) mất: chỉ ACK lại */

    uint8_t other[OTA_DATA_CHUNK]; memcpy(other, img, sizeof other); other[10] ^= 0xFF;
    xfer(f, ota_build_data(f, 0, other, OTA_DATA_CHUNK));
    expect_nack(OTA_T_DATA, OTA_E_BAD_SEQ);              /* trùng số nhưng khác nội dung */

    send_data(2, 1000); expect_nack(OTA_T_DATA, OTA_E_BAD_SEQ);           /* nhảy cóc */
    CHECK(*FL(OTA_APP_BASE + 256u) == 0xFF);                               /* không ghi */
    xfer(f, ota_build_data(f, 1, &img[128], 100)); expect_nack(OTA_T_DATA, OTA_E_BAD_LENGTH);
    CHECK(*FL(OTA_APP_BASE + 128u) == 0xFF);

    send_end(); expect_nack(OTA_T_END, OTA_E_INCOMPLETE);                  /* END sớm */
    for (uint16_t s = 1; s < 8; s++) { send_data(s, 1000); expect_ack_data(s); }
    send_end(); expect_ack(OTA_T_END);
    CHECK(valid_commits == 1);
    send_end(); expect_ack(OTA_T_END);                  /* ACK(END) mất, trong cửa sổ 2 s */
    CHECK(valid_commits == 1);                          /* chỉ commit một lần */
    send_begin(1000, crc); expect_nack(OTA_T_BEGIN, OTA_E_BAD_STATE);
}

static void test_image_errors(void)
{
    uint32_t crc = make_image(1000, OTA_SRAM_END);
    fresh(); full_session(1000, crc);                   /* có app hợp lệ trước */

    boot_protocol_init(); expect_ready();
    send_begin(OTA_APP_MAX_SIZE + 1u, crc); expect_nack(OTA_T_BEGIN, OTA_E_BAD_IMAGE_SIZE);
    send_begin(7, crc); expect_nack(OTA_T_BEGIN, OTA_E_BAD_IMAGE_SIZE);
    CHECK(boot_application_valid());                    /* app + metadata cũ còn nguyên */

    fresh();                                            /* CRC khai báo sai */
    send_begin(1000, crc ^ 1u); expect_ack(OTA_T_BEGIN);
    for (uint16_t s = 0; s < 8; s++) { send_data(s, 1000); expect_ack_data(s); }
    send_end(); expect_nack(OTA_T_END, OTA_E_IMAGE_CRC_ERROR);
    CHECK(boot_protocol_state() == BOOT_ST_WAIT_BEGIN && !boot_application_valid());

    crc = make_image(1000, 0x20005004u);                /* MSP ngoài SRAM */
    fresh();
    send_begin(1000, crc); expect_ack(OTA_T_BEGIN);
    for (uint16_t s = 0; s < 8; s++) { send_data(s, 1000); expect_ack_data(s); }
    send_end(); expect_nack(OTA_T_END, OTA_E_BAD_VECTOR);
    CHECK(valid_commits == 0 && !boot_application_valid());
}

static void test_timeout_and_state(void)
{
    uint32_t crc = make_image(1000, OTA_SRAM_END);
    fresh();
    send_data(0, 1000); expect_nack(OTA_T_DATA, OTA_E_BAD_STATE);
    send_end(); expect_nack(OTA_T_END, OTA_E_BAD_STATE);
    uint8_t unk[] = {0x55}; xfer(f, ota_encode(f, unk, 1)); expect_nack(0x55, OTA_E_UNSUPPORTED_TYPE);

    send_begin(1000, crc); expect_ack(OTA_T_BEGIN);
    now += BOOT_SESSION_TIMEOUT_MS + 1u;
    boot_protocol_poll();
    CHECK(boot_protocol_state() == BOOT_ST_WAIT_BEGIN);
    send_data(0, 1000); expect_nack(OTA_T_DATA, OTA_E_BAD_STATE);

    send_begin(1000, crc); expect_ack(OTA_T_BEGIN);     /* ENTER_UPDATE giữa phiên: huỷ, READY */
    send_data(0, 1000); expect_ack_data(0);
    xfer(f, ota_build_enter_update(f)); expect_ready();
    CHECK(boot_protocol_state() == BOOT_ST_WAIT_BEGIN);

    /* Frame hỏng không được xử lý */
    size_t n = ota_build_begin(f, 1000, crc); f[n - 1] ^= 1;
    xfer(f, n); expect_none();
    CHECK(boot_protocol_state() == BOOT_ST_WAIT_BEGIN);
}

int main(void)
{
    test_codec();
    test_sizes();
    test_lost_ready_and_begin_ack();
    test_duplicate_and_bad_data();
    test_image_errors();
    test_timeout_and_state();
    printf("OTA host tests: OK (%u checks)\n", checks);
    return 0;
}
