#!/usr/bin/env python3
"""Tạo metadata OTA cho lần nạp app đầu tiên bằng ST-Link (không qua ESP32).
   Dùng: python tools/make_meta.py build/gcc/firmware.bin build/gcc/meta.bin
   Nạp meta.bin tại 0x0800FC00 (OTA_META_BASE). Bố cục khớp bootloader/src/boot_validate.c:
     +0 size u32 LE, +4 image_crc32 u32 LE (CRC-32/ISO-HDLC), +8 valid u16 = 0xA55A."""
import struct
import sys
import zlib

APP_BASE, APP_LIMIT = 0x08002000, 0x0800FC00
SRAM_BASE, SRAM_END = 0x20000000, 0x20005000

if len(sys.argv) != 3:
    raise SystemExit(__doc__)
image = open(sys.argv[1], 'rb').read()
size = len(image)
if not 8 <= size <= APP_LIMIT - APP_BASE:
    raise SystemExit('Kích thước app %d B ngoài vùng app' % size)
msp, reset = struct.unpack_from('<II', image)
if msp & 7 or not SRAM_BASE < msp <= SRAM_END:
    raise SystemExit('MSP 0x%08X không hợp lệ' % msp)
if not reset & 1 or not APP_BASE <= (reset & ~1) < APP_BASE + size:
    raise SystemExit('Reset vector 0x%08X không nằm trong app: build sai địa chỉ (cần 0x08002000)?' % reset)
crc = zlib.crc32(image) & 0xFFFFFFFF
open(sys.argv[2], 'wb').write(struct.pack('<IIH', size, crc, 0xA55A))
print('meta: size=%d crc=0x%08X -> %s (nạp tại 0x0800FC00)' % (size, crc, sys.argv[2]))
