# Một source, hai môi trường

Không có `main_linux.c`/`main_keil.c`, không copy project vào hai thư mục độc lập.
Cùng code C11 và FreeRTOS GCC/ARM_CM3; chỉ compiler/linker/output khác nhau.
`startup.c` tự chọn nhánh GCC hoặc ARM Compiler 6 bằng `__ARMCC_VERSION`.

## Linux — GCC ARM

Trên Ubuntu, cài `gcc-arm-none-eabi`, `binutils-arm-none-eabi`, `libnewlib-arm-none-eabi`,
`make`, `python3`; `stlink-tools` nếu nạp bằng ST-Link. Ví dụ:

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi make python3 stlink-tools
make check-projects
make -j4
```

Kết quả ở `build/gcc/firmware.elf`, `.bin`, `.hex`, `.map`.
Dùng toolchain riêng: `make CROSS_COMPILE=/duong/dan/bin/arm-none-eabi-`.
Lệnh `make flash` ghi **0x08000000** bằng ST-Link; chỉ dùng cho bản standalone này.
Muốn debug dùng ELF với GDB/OpenOCD hoặc IDE bạn đã cấu hình; không commit đường dẫn máy riêng.

## Windows — Keil µVision 5 / ARM Compiler 6

1. Giải nén hoặc clone repo. Mở **`keil/MCU_Project.uvprojx`**.
2. Trong Pack Installer cài **Keil STM32F1xx_DFP** nếu máy chưa có hỗ trợ chip.
   Target Device là **STM32F103C8**, RAM 0x20000000/0x5000, ROM 0x08000000/0x10000.
3. Options for Target → Target → ARM Compiler: chọn **Use default compiler version 6**
   hoặc Compiler 6 đã cài. **Không dùng ARM Compiler 5. Tắt Use MicroLIB.**
4. Giữ custom scatter `keil/STM32F103C8.sct`, không bật auto memory layout thay nó.
   File project đã khai báo include path tương đối, `STM32F10X_MD` và các nhóm source.
5. **Không thêm startup/system từ RTE/CubeMX**: project đã có `startup.c` và vector table.
   Không thêm port RVDS hoặc heap khác; chỉ GCC/ARM_CM3 + heap_4.
6. Rebuild All. Output `build/keil/firmware.axf`, `.hex`.
   Xuất `.bin` khi cần: dùng `fromelf.exe` đi kèm chính ARM Compiler 6 đang chọn.
   After-build mặc định tắt để không phụ thuộc đường dẫn cài đặt Keil của từng máy.
   Có thể cấu hình Options → User → After Build với đường dẫn fromelf của máy đó,
   hoặc chạy từ thư mục `keil/` trong terminal đã có fromelf trên PATH:

```text
fromelf --bin --output ..\build\keil\firmware.bin ..\build\keil\firmware.axf
```

7. Debug → chọn **ST-Link Debugger**, SWD; Utilities → cùng ST-Link.
   Flash Download → chọn thuật toán STM32F10x 64 KiB phù hợp trong DFP đã cài.
   Tùy chọn probe/đường dẫn IDE cá nhân thuộc `.uvoptx/.uvguix.*` và đã bị gitignore.
8. Nạp và chạy; kiểm relay OFF trước khi kết nối tải thật. Chưa có UI sản phẩm trong bản khung.

Keil `.uvprojx` và `.sct` phải commit; `.uvoptx` không commit.
Bản này đã kiểm cấu trúc/path project; xem VALIDATION để biết build nào đã thật sự được chạy.

## Thêm file mới mà không lệch hai bản

Các file giao việc hiện tại đã được thêm sẵn. Khi cần file mới, leader thêm đường dẫn vào
`tools/sources.json` rồi chạy:

```bash
python3 tools/sync_projects.py
python3 tools/sync_projects.py --check
```

Windows có thể dùng `py -3` thay `python3`. Commit cả manifest, `tools/sources.mk`,
`keil/MCU_Project.uvprojx`. Không chỉ Add Existing Files trong Keil rồi quên Linux.
Script ghi lại project đã sinh; chỉnh thiết lập chung thì sửa script, không để cấu hình rơi mất lần sau.

## Bộ nhớ và OTA

Cả hai linker hiện cho application độc lập 64 KiB Flash / 20 KiB RAM, dự phòng C heap 512B,
main/ISR stack 1 KiB. FreeRTOS heap 10 KiB nằm trong `.bss` và khác C heap.
GCC tự copy `.data`/zero `.bss`; ARM Compiler 6 gọi `__main` để scatter-load rồi vào `main`.
Chưa build C++, constructors cho GCC startup chưa được hỗ trợ; ESP32 là project tách riêng.
Khi OTA được triển khai phải thêm target/linker bootloader + application có offset cho **cả hai** môi trường,
VTOR và địa chỉ nạp đồng bộ. Không sửa duy nhất một file linker.

## Tham chiếu kỹ thuật

- Port ARMClang: xem hướng dẫn chính thức được đóng gói trong FreeRTOS tại
  `middlewares/FreeRTOS-Kernel/portable/ARMClang/Use-the-GCC-ports.txt`.
- Arm scatter loading và vùng stack/heap:
  https://documentation-service.arm.com/static/63eb51fc9567172d4e2aa918
- Keil key sequence cho lệnh User:
  https://www.keil.com/support/man/docs/uv4cl/uv4cl_ut_keysequence.htm
