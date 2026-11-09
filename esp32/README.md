# ESP32 web OTA — chưa triển khai
Lê Trọng Mạnh phụ trách. Chọn đúng model ESP32 và framework khi bắt đầu; chưa có project build giả.
`src/main.cpp`: entry; `src/web_ota.cpp`: web upload; `src/uart_bridge.cpp`: protocol;
`data/index.html`: UI. STM32 PA9 TX -> ESP RX, PA10 RX <- ESP TX, logic 3.3V và chung GND.
Giao thức tại `docs/OTA_CONTRACT.md` cần được Mạnh chốt trước khi viết hai phía.
