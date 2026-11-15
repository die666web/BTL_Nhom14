/* ESP32 web OTA cho STM32 — Lê Trọng Mạnh. Xem docs/OTA_CONTRACT.md, docs/OTA_WORKFLOW.md.
 * ESP32 phát Wi-Fi riêng (AP). Kết nối Wi-Fi đó rồi mở http://192.168.4.1
 * Log: Serial (USB, 115200). UART nối STM32: Serial2 (uart_bridge.cpp). */
#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include "web_ota.h"
#include "uart_bridge.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Thieu include/secrets.h: copy include/secrets.h.example thanh include/secrets.h"
#endif

/* Log thiết bị vào/ra Wi-Fi của ESP32. Chạy trong task sự kiện Wi-Fi, chỉ in log. */
static void on_wifi_event(WiFiEvent_t event, WiFiEventInfo_t info)
{
    switch (event) {
    case ARDUINO_EVENT_WIFI_AP_START:
        Serial.println("[wifi] AP da bat");
        break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED: {
        const uint8_t *m = info.wifi_ap_staconnected.mac;
        Serial.printf("[wifi] + thiet bi ket noi  %02X:%02X:%02X:%02X:%02X:%02X (dang co %d)\n",
                      m[0], m[1], m[2], m[3], m[4], m[5], WiFi.softAPgetStationNum());
        break;
    }
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        Serial.printf("[wifi]   cap IP %s\n", IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
        break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: {
        const uint8_t *m = info.wifi_ap_stadisconnected.mac;
        Serial.printf("[wifi] - thiet bi ngat    %02X:%02X:%02X:%02X:%02X:%02X (con %d)\n",
                      m[0], m[1], m[2], m[3], m[4], m[5], WiFi.softAPgetStationNum());
        break;
    }
    default:
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    if (!LittleFS.begin(true)) {               /* true: format nếu FS hỏng/lần đầu */
        Serial.println("[fs] LittleFS loi");
    }
    if (!LittleFS.exists("/index.html")) {
        Serial.println("[fs] chua co index.html -> chay: pio run -t uploadfs");
    }

    WiFi.onEvent(on_wifi_event);
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(OTA_AP_SSID, OTA_AP_PASS)) {
        Serial.println("[wifi] bat AP loi (mat khau phai >= 8 ky tu?)");
    }
    Serial.printf("[wifi] AP %s, mo http://%s\n", OTA_AP_SSID, WiFi.softAPIP().toString().c_str());

    uart_bridge_begin();
    web_ota_begin();
}

void loop()
{
    web_ota_loop();
    uart_bridge_loop();
}
