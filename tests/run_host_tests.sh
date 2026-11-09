#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests
cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined -g \
  -Itests/host/mock -Ihal/inc -Ibsp -Iapp/inc \
  tests/host/test_drivers.c hal/src/hal_uart.c hal/src/hal_gpio.c \
  hal/src/hal_rcc.c hal/src/hal_dma.c -o build/tests/test_drivers
./build/tests/test_drivers
