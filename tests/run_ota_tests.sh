#!/usr/bin/env sh
# Test PC cho codec OTA + máy trạng thái bootloader (Flash/UART giả lập).
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/tests
cc -std=c11 -Wall -Wextra -Werror -fsanitize=undefined,address -g \
  -Icommon/ota_frame -Ibootloader/inc \
  tests/host/test_ota.c common/ota_frame/ota_frame.c \
  bootloader/src/boot_protocol.c bootloader/src/boot_validate.c \
  -o build/tests/test_ota
./build/tests/test_ota
