# Bootloader — chưa triển khai
Lê Trọng Mạnh phụ trách. `src/main.c` là entry point riêng, tuyệt đối không thêm vào application.
Chưa chốt kích thước bootloader; chưa có linker/build target để tránh nạp nhầm.
Application hiện ở 0x08000000, Flash 64 KiB/RAM 20 KiB; firmware hiện tại KHÔNG dùng để OTA.
Trước OTA: phân vùng boot/app/metadata; thêm linker `.ld` và `.sct` tương ứng, target riêng,
kiểm tra boundary/CRC/vector, cách request reset và clock handoff. Không giả định chip có 128 KiB.
