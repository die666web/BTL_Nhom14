/* Web upload .bin -> LittleFS, nút nạp STM32. Lê Trọng Mạnh.
 *   GET  /        trang upload (data/index.html, nạp bằng `pio run -t uploadfs`)
 *   GET  /status  JSON trạng thái file đang lưu + trạng thái nạp STM32
 *   POST /upload  multipart, field "fw" = file .bin. Chỉ LƯU, chưa nạp.
 *   POST /flash   bắt đầu nạp /fw.bin xuống STM32 (uart_bridge)
 * "Upload OK" chỉ nghĩa là ESP32 đã lưu file; trạng thái "xong" mới là STM32 đã ghi, đọc lại đúng và chạy app mới. */
#include <Arduino.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "web_ota.h"
#include "fw_store.h"
#include "uart_bridge.h"

static WebServer server(80);
static bool s_upload_ok;

static void send_json_error(int code, const char *msg)
{
    server.send(code, "application/json", String("{\"ok\":false,\"error\":\"") + msg + "\"}");
}

static void handle_upload_chunk(void)
{
    HTTPUpload &up = server.upload();
    switch (up.status) {
    case UPLOAD_FILE_START:
        Serial.printf("[web] nhan %s\n", up.filename.c_str());
        s_upload_ok = !uart_bridge_busy() && fw_store_begin();
        break;
    case UPLOAD_FILE_WRITE:
        if (s_upload_ok) s_upload_ok = fw_store_write(up.buf, up.currentSize);
        break;
    case UPLOAD_FILE_END:
        if (s_upload_ok) s_upload_ok = fw_store_end();
        break;
    case UPLOAD_FILE_ABORTED:
        fw_store_abort();
        s_upload_ok = false;
        break;
    }
}

static void handle_upload_done(void)
{
    if (uart_bridge_busy()) return send_json_error(409, "dang nap STM32, thu lai sau");
    if (!s_upload_ok) return send_json_error(400, fw_store_last_error());
    FwInfo fw = fw_store_info();
    char buf[96];
    snprintf(buf, sizeof buf, "{\"ok\":true,\"size\":%lu,\"crc\":\"%08lX\"}",
             (unsigned long)fw.size, (unsigned long)fw.crc);
    server.send(200, "application/json", buf);
}

static void handle_status(void)
{
    FwInfo fw = fw_store_info();
    char buf[320];
    snprintf(buf, sizeof buf,
             "{\"fw\":%s,\"size\":%lu,\"crc\":\"%08lX\",\"prev\":%s,"
             "\"fs_free\":%lu,\"busy\":%s,\"stm32\":\"%s\"}",
             fw.present ? "true" : "false", (unsigned long)fw.size, (unsigned long)fw.crc,
             fw.has_prev ? "true" : "false",
             (unsigned long)(LittleFS.totalBytes() - LittleFS.usedBytes()),
             uart_bridge_busy() ? "true" : "false",
             uart_bridge_state_text());
    server.send(200, "application/json", buf);
}

static void handle_flash(void)
{
    if (!fw_store_info().present) return send_json_error(400, "chua co firmware");
    if (uart_bridge_busy()) return send_json_error(409, "dang nap");
    if (!uart_bridge_start(FW_PATH_CUR)) return send_json_error(500, uart_bridge_state_text());
    server.send(202, "application/json", "{\"ok\":true}");
}

void web_ota_begin(void)
{
    server.on("/upload", HTTP_POST, handle_upload_done, handle_upload_chunk);
    server.on("/status", HTTP_GET, handle_status);
    server.on("/flash", HTTP_POST, handle_flash);
    server.serveStatic("/", LittleFS, "/index.html");
    server.onNotFound([] { server.send(404, "text/plain", "not found"); });
    server.begin();
}

void web_ota_loop(void) { server.handleClient(); }
