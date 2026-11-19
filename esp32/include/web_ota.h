#ifndef WEB_OTA_H
#define WEB_OTA_H
void web_ota_begin(void);   /* gọi sau khi Wi-Fi + LittleFS sẵn sàng */
void web_ota_loop(void);    /* gọi liên tục trong loop() */
#endif
