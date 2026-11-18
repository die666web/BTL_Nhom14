#!/usr/bin/env sh
# Chạy toàn bộ test PC (Linux + gcc): HAL (UART/GPIO/relay/nút) và OTA (codec + bootloader).
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests

cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined -g \
  -Itests/host/mock -Ihal/inc -Ibsp -Iapp/inc \
  tests/host/test_hal.c hal/src/hal_uart.c hal/src/hal_gpio.c hal/src/hal_rcc.c \
  hal/src/relay.c hal/src/button.c -o build/tests/test_hal
./build/tests/test_hal

cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -g \
  -Icommon/ota_frame -Ibootloader/inc \
  tests/host/test_ota.c common/ota_frame/ota_frame.c \
  bootloader/src/boot_protocol.c bootloader/src/boot_validate.c -o build/tests/test_ota
./build/tests/test_ota
