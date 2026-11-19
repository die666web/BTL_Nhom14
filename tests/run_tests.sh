#!/usr/bin/env sh
# Chạy test PC (Linux + gcc) cho HAL: UART, GPIO, relay, nút nhấn.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests

cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined -g \
  -Itests/host/mock -Ihal/inc -Ibsp -Iapp/inc \
  tests/host/test_hal.c hal/src/hal_uart.c hal/src/hal_gpio.c hal/src/hal_rcc.c \
  hal/src/relay.c hal/src/button.c -o build/tests/test_hal
./build/tests/test_hal

