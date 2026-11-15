/* uart_bridge — phía gửi OTA UART v1 (docs/OTA_CONTRACT.md mục 6). Lê Trọng Mạnh.
 *
 *   IDLE -> WAIT_READY -> WAIT_BEGIN_ACK -> WAIT_DATA_ACK (lặp) -> WAIT_END_ACK -> DONE
 *                                     bất kỳ NACK đúng lệnh / hết retry -> FAILED
 *
 * Stop-and-wait: gửi trọn 1 frame, chờ đúng phản hồi, mới gửi frame tiếp.
 * Mỗi yêu cầu: 1 lần gửi + tối đa 3 lần gửi lại (nguyên frame). Timeout tính từ khi byte cuối ra dây.
 * Log ra Serial (USB), UART OTA là Serial2 riêng. */
#include <Arduino.h>
#include <stdarg.h>
#include <LittleFS.h>
#include "uart_bridge.h"
#include "ota_frame.h"
#include "ota_layout.h"

#define OTA_UART_BAUD     115200
#define OTA_UART_RX_PIN   16      /* ESP32 RX  <- STM32 PA9 (TX)  */
#define OTA_UART_TX_PIN   17      /* ESP32 TX  -> STM32 PA10 (RX) */

#define TO_READY_MS   1000u
#define TO_BEGIN_MS   3000u       /* bao gồm thời gian STM32 xoá Flash */
#define TO_DATA_MS     100u
#define TO_END_MS      500u
#define MAX_RETRY        3u

static HardwareSerial &ota = Serial2;

enum State { IDLE, WAIT_READY, WAIT_BEGIN_ACK, WAIT_DATA_ACK, WAIT_END_ACK, DONE, FAILED };

static State        s_state = IDLE;
static File         s_file;
static uint32_t     s_size, s_crc, s_sent;
static uint16_t     s_seq;
static uint8_t      s_chunk;             /* số byte của gói DATA đang chờ ACK */
static uint8_t      s_tx[OTA_FRAME_MAX];
static size_t       s_txlen;
static uint32_t     s_t0;
static uint8_t      s_retry;
static ota_parser_t s_rx;
static char         s_text[80] = "san sang";

static const char *state_name(State s)
{
    switch (s) {
    case WAIT_READY:     return "READY";
    case WAIT_BEGIN_ACK: return "ACK(BEGIN)";
    case WAIT_DATA_ACK:  return "ACK(DATA)";
    case WAIT_END_ACK:   return "ACK(END)";
    default:             return "?";
    }
}

static uint32_t timeout_of(State s)
{
    switch (s) {
    case WAIT_READY:     return TO_READY_MS;
    case WAIT_BEGIN_ACK: return TO_BEGIN_MS;
    case WAIT_DATA_ACK:  return TO_DATA_MS;
    default:             return TO_END_MS;
    }
}

static uint8_t expected_type(State s)
{
    switch (s) {
    case WAIT_READY:     return OTA_T_ENTER_UPDATE;
    case WAIT_BEGIN_ACK: return OTA_T_BEGIN;
    case WAIT_DATA_ACK:  return OTA_T_DATA;
    default:             return OTA_T_END;
    }
}

static void set_text(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_text, sizeof s_text, fmt, ap);
    va_end(ap);
    Serial.printf("[ota] %s\n", s_text);
}

static void finish(State st)
{
    if (s_file) s_file.close();
    s_state = st;
}

/* Gửi s_tx, chờ byte cuối ra dây rồi mới bắt đầu đếm timeout. */
static void transmit(void)
{
    ota.write(s_tx, s_txlen);
    ota.flush();
    s_t0 = millis();
}

static void send_new(State next, size_t len)
{
    s_txlen = len;
    s_retry = 0;
    s_state = next;
    transmit();
}

static bool send_data(void)
{
    uint32_t remain = s_size - s_sent;
    s_chunk = (uint8_t)(remain < OTA_DATA_CHUNK ? remain : OTA_DATA_CHUNK);
    uint8_t buf[OTA_DATA_CHUNK];
    if (!s_file.seek((uint32_t)s_seq * OTA_DATA_CHUNK) || s_file.read(buf, s_chunk) != s_chunk) {
        set_text("loi: doc file LittleFS");
        finish(FAILED);
        return false;
    }
    send_new(WAIT_DATA_ACK, ota_build_data(s_tx, s_seq, buf, s_chunk));
    if (s_seq % 40u == 0u)
        set_text("dang nap %lu%% (%lu/%lu B)", (unsigned long)(s_sent * 100u / s_size),
                 (unsigned long)s_sent, (unsigned long)s_size);
    return true;
}

void uart_bridge_begin(void)
{
    ota.begin(OTA_UART_BAUD, SERIAL_8N1, OTA_UART_RX_PIN, OTA_UART_TX_PIN);
    ota_parser_reset(&s_rx);
}

bool uart_bridge_busy(void) { return s_state >= WAIT_READY && s_state <= WAIT_END_ACK; }

const char *uart_bridge_state_text(void) { return s_text; }

bool uart_bridge_start(const char *path)
{
    if (uart_bridge_busy()) return false;
    s_file = LittleFS.open(path, "r");
    if (!s_file) { set_text("loi: khong mo duoc %s", path); s_state = FAILED; return false; }
    s_size = s_file.size();
    if (s_size < 8u || s_size > OTA_APP_MAX_SIZE) {
        set_text("loi: kich thuoc %lu B ngoai vung app", (unsigned long)s_size);
        finish(FAILED);
        return false;
    }

    /* CRC-32 tính lại trên đúng file sẽ gửi (không tin metadata cũ). */
    uint32_t crc = OTA_CRC32_INIT;
    uint8_t buf[256];
    size_t n;
    while ((n = s_file.read(buf, sizeof buf)) > 0) crc = ota_crc32_update(crc, buf, n);
    s_crc = ota_crc32_final(crc);

    s_sent = 0;
    s_seq = 0;
    while (ota.available()) ota.read();       /* bỏ rác cũ */
    ota_parser_reset(&s_rx);
    set_text("dang vao bootloader (%lu B, crc %08lX)", (unsigned long)s_size, (unsigned long)s_crc);
    send_new(WAIT_READY, ota_build_enter_update(s_tx));
    return true;
}

static void on_nack(const uint8_t *b, uint8_t len)
{
    if (len != 5u || b[1] != expected_type(s_state)) return;   /* NACK của lệnh khác: bỏ qua */
    uint16_t seq = ota_get_u16(&b[3]);
    if (s_state == WAIT_DATA_ACK && seq != s_seq && seq != OTA_SEQ_NONE) return;
    set_text("loi: STM32 tu choi %s: %s", state_name(s_state), ota_error_name(b[2]));
    finish(FAILED);
}

static void on_frame(const uint8_t *b, uint8_t len)
{
    if (b[0] == OTA_T_NACK) { on_nack(b, len); return; }

    switch (s_state) {
    case WAIT_READY:
        if (b[0] == OTA_T_READY && len == 1u) {
            set_text("bootloader san sang, dang xoa flash");
            send_new(WAIT_BEGIN_ACK, ota_build_begin(s_tx, s_size, s_crc));
        }
        break;
    case WAIT_BEGIN_ACK:
        if (b[0] == OTA_T_ACK && len == 2u && b[1] == OTA_T_BEGIN) send_data();
        break;
    case WAIT_DATA_ACK:
        /* ACK cũ (seq khác) bị bỏ qua và KHÔNG khởi động lại bộ đếm thời gian. */
        if (b[0] == OTA_T_ACK && len == 4u && b[1] == OTA_T_DATA && ota_get_u16(&b[2]) == s_seq) {
            s_sent += s_chunk;
            s_seq++;
            if (s_sent >= s_size) {
                set_text("dang kiem tra tren STM32");
                send_new(WAIT_END_ACK, ota_build_end(s_tx));
            } else {
                send_data();
            }
        }
        break;
    case WAIT_END_ACK:
        if (b[0] == OTA_T_ACK && len == 2u && b[1] == OTA_T_END) {
            set_text("xong: firmware da qua kiem tra va duoc commit (%lu B)", (unsigned long)s_size);
            finish(DONE);
        }
        break;
    default:
        break;
    }
}

void uart_bridge_loop(void)
{
    while (ota.available()) {
        uint8_t byte = (uint8_t)ota.read();
        if (ota_parser_feed(&s_rx, byte, millis()))
            on_frame(s_rx.body, s_rx.len);
    }

    if (!uart_bridge_busy()) return;
    if ((uint32_t)(millis() - s_t0) <= timeout_of(s_state)) return;

    if (s_retry < MAX_RETRY) {
        s_retry++;
        Serial.printf("[ota] het %lu ms cho %s, gui lai lan %u\n",
                      (unsigned long)timeout_of(s_state), state_name(s_state), s_retry);
        transmit();
        return;
    }
    if (s_state == WAIT_END_ACK)
        set_text("chua xac nhan duoc ket qua (mat ACK END) - kiem tra STM32");
    else
        set_text("loi: het thoi gian cho %s", state_name(s_state));
    finish(FAILED);
}
