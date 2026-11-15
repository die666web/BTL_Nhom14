/* Console debug qua USB Serial. Xem include/debug_console.h. Lê Trọng Mạnh. */
#include <Arduino.h>
#include <LittleFS.h>
#include "debug_console.h"
#include "fw_store.h"
#include "uart_bridge.h"

#define PUT_CHUNK       256u
#define PUT_TIMEOUT_MS  3000u

static char   s_line[64];
static size_t s_len;

static void cmd_ls(void)
{
    File root = LittleFS.open("/");
    for (File f = root.openNextFile(); f; f = root.openNextFile())
        Serial.printf("  %-16s %6u B\n", f.name(), (unsigned)f.size());
    Serial.printf("  free %u / %u B\n", (unsigned)(LittleFS.totalBytes() - LittleFS.usedBytes()),
                  (unsigned)LittleFS.totalBytes());
}

static void cmd_status(void)
{
    FwInfo fw = fw_store_info();
    Serial.printf("fw=%s size=%lu crc=%08lX prev=%s stm32=\"%s\" busy=%d\n",
                  fw.present ? "yes" : "no", (unsigned long)fw.size, (unsigned long)fw.crc,
                  fw.has_prev ? "yes" : "no", uart_bridge_state_text(), uart_bridge_busy());
}

/* Nhận file nhị phân có điều khiển luồng: mỗi khối PUT_CHUNK byte trả "." */
static void cmd_put(unsigned long size)
{
    if (uart_bridge_busy()) { Serial.println("PUT ERR dang nap STM32"); return; }
    if (size == 0) { Serial.println("PUT ERR size"); return; }
    if (!fw_store_begin()) { Serial.printf("PUT ERR %s\n", fw_store_last_error()); return; }
    Serial.println("PUT READY");

    uint8_t buf[PUT_CHUNK];
    unsigned long got = 0;
    bool ok = true;
    while (got < size) {
        size_t want = (size - got) < PUT_CHUNK ? (size_t)(size - got) : PUT_CHUNK;
        size_t n = 0;
        uint32_t t0 = millis();
        while (n < want && millis() - t0 < PUT_TIMEOUT_MS) {
            int c = Serial.read();
            if (c >= 0) { buf[n++] = (uint8_t)c; t0 = millis(); }
            else delay(1);
        }
        if (n < want) { fw_store_abort(); Serial.printf("PUT ERR timeout sau %lu B\n", got + n); return; }
        if (ok) ok = fw_store_write(buf, n);       /* lỗi vẫn đọc hết để PC không bị treo */
        got += n;
        Serial.print('.');
    }
    Serial.println();
    if (ok) ok = fw_store_end();
    if (!ok) { Serial.printf("PUT ERR %s\n", fw_store_last_error()); return; }
    FwInfo fw = fw_store_info();
    Serial.printf("PUT OK %lu %08lX\n", (unsigned long)fw.size, (unsigned long)fw.crc);
}

static void run(char *cmd)
{
    if (!strcmp(cmd, "help")) {
        Serial.println("lenh: help | ls | status | put <size> | flash");
    } else if (!strcmp(cmd, "ls")) {
        cmd_ls();
    } else if (!strcmp(cmd, "status")) {
        cmd_status();
    } else if (!strncmp(cmd, "put ", 4)) {
        cmd_put(strtoul(cmd + 4, NULL, 10));
    } else if (!strcmp(cmd, "flash")) {
        if (!fw_store_info().present) Serial.println("FLASH ERR chua co firmware");
        else Serial.println(uart_bridge_start(FW_PATH_CUR) ? "FLASH STARTED" : "FLASH ERR");
    } else if (cmd[0]) {
        Serial.printf("lenh khong ro: %s (go help)\n", cmd);
    }
}

void debug_console_loop(void)
{
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            s_line[s_len] = '\0';
            run(s_line);
            s_len = 0;
        } else if (s_len < sizeof s_line - 1) {
            s_line[s_len++] = c;
        }
    }
}
