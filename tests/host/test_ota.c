/* Test PC cho OTA v1: codec frame (common/ota_frame) + máy trạng thái bootloader
 * (bootloader/src/boot_protocol.c, boot_validate.c) chạy trên Flash/UART giả lập.
 * Chạy: sh tests/run_tests.sh   (Linux, cần gcc)
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
static unsigned valid_commits;   /* số lần ghi cờ VALID của metadata */
#define FL(a) (&flash[(a) - OTA_FLASH_BASE])

const uint8_t *boot_flash_ptr(uint32_t a) { return FL(a); }

bool boot_flash_erase_page(uint32_t a)
{
    if (a % OTA_PAGE_SIZE || a < OTA_APP_BASE || a >= OTA_FLASH_END) return false;
    memset(FL(a), 0xFF, OTA_PAGE_SIZE);
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
    valid_commits = 0;
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

/* ===================== Test ===================== */

/* 1. Codec: CRC16/CRC32 đúng vector chuẩn "123456789"; frame ENTER_UPDATE đúng từng byte;
 *    frame sai CRC bị bỏ im lặng, frame đúng phía sau vẫn đọc được. */
static void test_codec(void)
{
    static const uint8_t ascii[] = "123456789";
    CHECK(ota_crc16(ascii, 9) == 0x29B1u);
    CHECK(ota_crc32_final(ota_crc32_update(OTA_CRC32_INIT, ascii, 9)) == 0xCBF43926u);

    static const uint8_t enter[] = {0xA5, 0x5A, 0x01, 0x01, 0x1F, 0x3E};
    uint8_t out[OTA_FRAME_MAX];
    CHECK(ota_build_enter_update(out) == sizeof enter && !memcmp(out, enter, sizeof enter));

    uint8_t bad[sizeof enter];
    memcpy(bad, enter, sizeof bad);
    bad[5] ^= 1;                                         /* làm hỏng byte CRC */
    ota_parser_t p;
    ota_parser_reset(&p);
    int frames = 0;
    for (size_t i = 0; i < sizeof bad; i++)   frames += ota_parser_feed(&p, bad[i], 0);
    CHECK(frames == 0);
    for (size_t i = 0; i < sizeof enter; i++) frames += ota_parser_feed(&p, enter[i], 0);
    CHECK(frames == 1 && p.body[0] == OTA_T_ENTER_UPDATE);
}

/* 2. Phiên nạp đầy đủ BEGIN -> DATA... -> END với ảnh 1001 B (gói cuối lẻ, đệm 0xFF):
 *    bootloader commit metadata và chuyển sang chạy app. */
static void test_full_session(void)
{
    uint32_t crc = make_image(1001, OTA_SRAM_END);
    fresh();
    full_session(1001, crc);
    CHECK(*FL(OTA_APP_BASE + 1000u) == img[1000]);       /* byte cuối của ảnh */
    CHECK(*FL(OTA_APP_BASE + 1001u) == 0xFF);            /* byte đệm */
    CHECK(valid_commits == 1);
}

/* 3. Mất ACK: ESP32 gửi lại DATA/END cũ -> chỉ ACK lại, không ghi hay commit lần hai.
 *    Gói nhảy cóc số thứ tự bị NACK BAD_SEQ và không ghi xuống Flash. */
static void test_retransmit(void)
{
    uint32_t crc = make_image(1000, OTA_SRAM_END);
    fresh();
    send_begin(1000, crc); expect_ack(OTA_T_BEGIN);
    send_data(0, 1000);    expect_ack_data(0);
    send_data(0, 1000);    expect_ack_data(0);           /* gói trùng: ACK lại */
    send_data(2, 1000);    expect_nack(OTA_T_DATA, OTA_E_BAD_SEQ);
    CHECK(*FL(OTA_APP_BASE + 2u * OTA_DATA_CHUNK) == 0xFF);
    for (uint16_t s = 1; s < 8; s++) { send_data(s, 1000); expect_ack_data(s); }
    send_end(); expect_ack(OTA_T_END);
    send_end(); expect_ack(OTA_T_END);                   /* END trùng: ACK lại */
    CHECK(valid_commits == 1);                           /* commit đúng một lần */
}

/* 4. Ảnh hỏng: CRC khai báo không khớp dữ liệu -> NACK IMAGE_CRC_ERROR, không commit,
 *    bootloader quay về chờ BEGIN (app cũ không bị coi là hợp lệ). */
static void test_bad_image(void)
{
    uint32_t crc = make_image(1000, OTA_SRAM_END);
    fresh();
    send_begin(1000, crc ^ 1u); expect_ack(OTA_T_BEGIN);
    for (uint16_t s = 0; s < 8; s++) { send_data(s, 1000); expect_ack_data(s); }
    send_end(); expect_nack(OTA_T_END, OTA_E_IMAGE_CRC_ERROR);
    CHECK(valid_commits == 0);
    CHECK(boot_protocol_state() == BOOT_ST_WAIT_BEGIN && !boot_application_valid());
}

int main(void)
{
    test_codec();
    test_full_session();
    test_retransmit();
    test_bad_image();
    printf("OTA host tests: OK (%u checks)\n", checks);
    return 0;
}
