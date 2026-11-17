#!/usr/bin/env python3
"""Công cụ test OTA v1 từ máy tính (Windows/Linux). Đặc tả: docs/OTA_CONTRACT.md.

  make-bin OUT [--size N]            tạo app.bin giả hợp lệ (vector ở 0x08002000) để test
  http-put FILE [--host H] [--flash] upload lên web ESP32 như trình duyệt, đo thời gian, theo dõi /status
  serial-put PORT FILE [--flash]     upload qua console USB của ESP32 (không cần Wi-Fi)
  fake-stm32 PORT [--fail-every N]   giả lập app + bootloader STM32 trên cổng USB-UART nối
                                     ESP32 GPIO17(TX)->RX adapter, GPIO16(RX)<-TX adapter, chung GND

Serial cần pyserial (có sẵn trong Python của PlatformIO: ~/.platformio/penv)."""
import argparse
import json
import struct
import sys
import time
import urllib.request
import uuid
import zlib

APP_BASE, APP_LIMIT = 0x08002000, 0x0800FC00
SRAM_END = 0x20005000
T_ENTER, T_READY, T_BEGIN, T_DATA, T_END, T_ACK, T_NACK = 0x01, 0x02, 0x10, 0x11, 0x12, 0x70, 0x71
ERR = {1: 'BAD_STATE', 2: 'BAD_LENGTH', 3: 'BAD_IMAGE_SIZE', 4: 'BAD_SEQ', 5: 'FLASH_ERROR',
       6: 'INCOMPLETE', 7: 'IMAGE_CRC_ERROR', 8: 'BAD_VECTOR', 9: 'UNSUPPORTED_TYPE'}
NAMES = {T_ENTER: 'ENTER_UPDATE', T_READY: 'READY', T_BEGIN: 'BEGIN', T_DATA: 'DATA',
         T_END: 'END', T_ACK: 'ACK', T_NACK: 'NACK'}


# ---------------- codec (giống common/ota_frame/ota_frame.c) ----------------
def crc16(data, crc=0xFFFF):
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def encode(body):
    body = bytes(body)
    assert 1 <= len(body) <= 131
    lb = bytes([len(body)]) + body
    return b'\xA5\x5A' + lb + struct.pack('<H', crc16(lb))


class Parser:
    def __init__(self):
        self.reset()

    def reset(self):
        self.st, self.buf, self.need, self.last = 0, b'', 0, 0.0

    def feed(self, b, now):
        if self.st and now - self.last > 0.020:
            self.reset()
        self.last = now
        if self.st == 0:
            self.st = 1 if b == 0xA5 else 0
        elif self.st == 1:
            self.st = 2 if b == 0x5A else (1 if b == 0xA5 else 0)
        elif self.st == 2:
            if 1 <= b <= 131:
                self.need, self.buf, self.st = b + 2, bytes([b]), 3
            else:
                self.reset()
        else:
            self.buf += bytes([b])
            if len(self.buf) == self.need + 1:
                lb, crc = self.buf[:-2], struct.unpack('<H', self.buf[-2:])[0]
                self.reset()
                if crc16(lb) == crc:
                    return lb[1:]
        return None


# ---------------- make-bin ----------------
def make_image(size):
    img = bytearray((i * 7 + 3) & 0xFF for i in range(size))
    struct.pack_into('<II', img, 0, SRAM_END, APP_BASE + 9)
    return bytes(img)


def cmd_make_bin(a):
    img = make_image(a.size)
    open(a.out, 'wb').write(img)
    print('Đã tạo %s: %d B, crc32 %08X' % (a.out, len(img), zlib.crc32(img) & 0xFFFFFFFF))


# ---------------- http-put ----------------
def http(host, path, data=None, ctype=None, timeout=30):
    req = urllib.request.Request('http://%s%s' % (host, path), data=data, method='POST' if data is not None else 'GET')
    if ctype:
        req.add_header('Content-Type', ctype)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as r:
            return r.status, r.read().decode('utf-8', 'replace')
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode('utf-8', 'replace')


def watch_status(host, seconds=120):
    last = None
    t0 = time.time()
    while time.time() - t0 < seconds:
        code, body = http(host, '/status', timeout=10)
        st = json.loads(body).get('stm32') if code == 200 else 'HTTP %d' % code
        if st != last:
            print('  [%5.1fs] stm32: %s' % (time.time() - t0, st))
            last = st
        if st and (st.startswith('xong') or st.startswith('loi') or st.startswith('chua')):
            return
        time.sleep(0.5)


def cmd_http_put(a):
    img = open(a.file, 'rb').read()
    print('GET /status ...', end=' ', flush=True)
    t = time.time()
    code, body = http(a.host, '/status', timeout=10)
    print('%d trong %.2fs: %s' % (code, time.time() - t, body))
    boundary = uuid.uuid4().hex
    data = (('--%s\r\nContent-Disposition: form-data; name="fw"; filename="%s"\r\n'
             'Content-Type: application/octet-stream\r\n\r\n' % (boundary, a.file.split('/')[-1].split('\\')[-1])).encode()
            + img + ('\r\n--%s--\r\n' % boundary).encode())
    print('POST /upload %d B ...' % len(img), end=' ', flush=True)
    t = time.time()
    code, body = http(a.host, '/upload', data, 'multipart/form-data; boundary=' + boundary, timeout=60)
    print('%d trong %.2fs: %s' % (code, time.time() - t, body))
    if code == 200 and a.flash:
        print('POST /flash:', http(a.host, '/flash', b'')[1])
        watch_status(a.host)


# ---------------- serial ----------------
def open_serial(port):
    import serial
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, 115200, 0.1
    s.dtr = s.rts = False          # không reset ESP32 khi mở cổng
    s.open()
    return s


def read_until(s, prefixes, timeout):
    t0 = time.time()
    buf = b''
    while time.time() - t0 < timeout:
        buf += s.read(256)
        while b'\n' in buf:
            line, buf = buf.split(b'\n', 1)
            text = line.decode('utf-8', 'replace').strip()
            if text:
                print('  esp32>', text)
            for p in prefixes:
                if text.startswith(p):
                    return text
    return None


def cmd_serial_put(a):
    img = open(a.file, 'rb').read()
    s = open_serial(a.port)
    s.reset_input_buffer()
    s.write(b'\nput %d\n' % len(img))
    if not read_until(s, ['PUT READY', 'PUT ERR'], 3) or False:
        sys.exit('ESP32 không trả PUT READY (firmware cũ chưa có console?)')
    t = time.time()
    for i in range(0, len(img), 256):
        s.write(img[i:i + 256])
        d = s.read(1)
        t1 = time.time()
        while d != b'.' and time.time() - t1 < 3:
            d = s.read(1)
        if d != b'.':
            sys.exit('Mất "." sau %d B' % i)
    res = read_until(s, ['PUT OK', 'PUT ERR'], 5)
    print('Kết quả: %s (%.2fs, crc PC %08X)' % (res, time.time() - t, zlib.crc32(img) & 0xFFFFFFFF))
    if res and res.startswith('PUT OK') and a.flash:
        s.write(b'flash\n')
        read_until(s, ['[ota] xong', '[ota] loi', '[ota] chua'], 120)
    s.close()


# ---------------- fake-stm32 ----------------
def cmd_fake_stm32(a):
    """Giả lập app (chờ ENTER_UPDATE) + bootloader theo mục 5 của đặc tả, Flash là bytearray."""
    s = open_serial(a.port)
    p = Parser()
    flash = bytearray(b'\xFF' * (APP_LIMIT - APP_BASE))
    state, size, crc, written, next_seq, count = 'APP', 0, 0, 0, 0, 0

    def send(body, note=''):
        s.write(encode(body))
        print('  stm32 -> %s %s' % (NAMES.get(body[0], hex(body[0])), note))

    def nack(t, e, seq=0xFFFF):
        send(bytes([T_NACK, t, e]) + struct.pack('<H', seq), '%s %s' % (NAMES.get(t), ERR[e]))
        return 'WAIT_BEGIN' if e in (5, 7, 8) else state

    print('fake-stm32 trên %s: trạng thái APP, chờ ENTER_UPDATE (Ctrl+C để thoát)' % a.port)
    while True:
        data = s.read(256)
        now = time.time()
        for b in data:
            body = p.feed(b, now)
            if body is None:
                continue
            t = body[0]
            count += 1
            if a.fail_every and count % a.fail_every == 0:
                print('  (bỏ qua frame %s để thử retry)' % NAMES.get(t, hex(t)))
                continue
            if t == T_DATA and len(body) >= 3:
                print('  esp32 -> DATA seq %d (%d B)' % (struct.unpack_from('<H', body, 1)[0], len(body) - 3))
            else:
                print('  esp32 -> %s' % NAMES.get(t, hex(t)))
            if state == 'APP':
                if t == T_ENTER:
                    print('  [app] ENTER_UPDATE -> reset vào bootloader')
                    time.sleep(0.05)
                    state = 'WAIT_BEGIN'
                    send([T_READY])
                continue
            if t == T_ENTER:
                state = 'WAIT_BEGIN'
                send([T_READY])
            elif t == T_BEGIN and len(body) == 9:
                bsize, bcrc = struct.unpack_from('<II', body, 1)
                if state == 'RECEIVING' and next_seq == 0 and (bsize, bcrc) == (size, crc):
                    send([T_ACK, T_BEGIN], '(lặp)')
                elif state != 'WAIT_BEGIN':
                    state = nack(T_BEGIN, 1)
                elif not 8 <= bsize <= len(flash):
                    state = nack(T_BEGIN, 3)
                else:
                    size, crc, written, next_seq = bsize, bcrc, 0, 0
                    flash[:] = b'\xFF' * len(flash)
                    time.sleep(0.02 * ((size + 1023) // 1024))   # giả lập thời gian xoá
                    state = 'RECEIVING'
                    send([T_ACK, T_BEGIN], 'size %d crc %08X' % (size, crc))
            elif t == T_DATA and len(body) >= 4:
                seq, payload = struct.unpack_from('<H', body, 1)[0], body[3:]
                if state != 'RECEIVING':
                    state = nack(T_DATA, 1, seq)
                elif seq == next_seq:
                    exp = min(128, size - written)
                    if exp <= 0:
                        state = nack(T_DATA, 4, seq)
                    elif len(payload) != exp:
                        state = nack(T_DATA, 2, seq)
                    else:
                        flash[written:written + exp] = payload
                        written += exp
                        next_seq += 1
                        send([T_ACK, T_DATA] + list(struct.pack('<H', seq)), 'seq %d' % seq)
                elif next_seq and seq == next_seq - 1 and bytes(flash[seq * 128:written]) == payload:
                    send([T_ACK, T_DATA] + list(struct.pack('<H', seq)), 'seq %d (lặp)' % seq)
                else:
                    state = nack(T_DATA, 4, seq)
            elif t == T_END and len(body) == 1:
                if state == 'GRACE':
                    send([T_ACK, T_END], '(lặp)')
                elif state != 'RECEIVING':
                    state = nack(T_END, 1)
                elif written < size:
                    state = nack(T_END, 6)
                elif zlib.crc32(bytes(flash[:size])) & 0xFFFFFFFF != crc:
                    state = nack(T_END, 7)
                else:
                    msp, rv = struct.unpack_from('<II', flash, 0)
                    if msp & 7 or not 0x20000000 < msp <= SRAM_END or not rv & 1 or not APP_BASE <= (rv & ~1) < APP_BASE + size:
                        state = nack(T_END, 8)
                    else:
                        state = 'GRACE'
                        grace_t = time.time()
                        send([T_ACK, T_END], 'commit %d B' % size)
            else:
                state = nack(t, 9 if t not in NAMES else 1)
        if state == 'GRACE' and time.time() - grace_t >= 2:
            print('  [boot] hết cửa sổ 2 s -> chạy app mới (OK)')
            state = 'APP'


def main():
    try:                                   # console Windows (cp1252) không in được tiếng Việt
        sys.stdout.reconfigure(encoding='utf-8', errors='replace', line_buffering=True)
    except AttributeError:
        pass
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    m = sub.add_parser('make-bin'); m.add_argument('out'); m.add_argument('--size', type=int, default=1001)
    h = sub.add_parser('http-put'); h.add_argument('file'); h.add_argument('--host', default='192.168.4.1'); h.add_argument('--flash', action='store_true')
    s = sub.add_parser('serial-put'); s.add_argument('port'); s.add_argument('file'); s.add_argument('--flash', action='store_true')
    f = sub.add_parser('fake-stm32'); f.add_argument('port'); f.add_argument('--fail-every', type=int, default=0)
    a = ap.parse_args()
    {'make-bin': cmd_make_bin, 'http-put': cmd_http_put, 'serial-put': cmd_serial_put, 'fake-stm32': cmd_fake_stm32}[a.cmd](a)


if __name__ == '__main__':
    main()
