## Phần việc / người thực hiện
- Nhánh feature/... -> develop
- File đã sửa:
- API thay đổi (nếu có, leader duyệt):

## Kiểm tra
- [ ] Linux build không warning / hoặc ghi rõ chưa chạy
- [ ] Keil ARM Compiler 6 build không warning / hoặc ghi rõ chưa chạy
- [ ] Source manifest và project cùng danh sách file
- [ ] Test trên kit: bước, kết quả, ảnh/log trong tests/manual/
- [ ] Timeout, xử lý lỗi, ISR FromISR, không delay bận trong task
- [ ] Stack high-water mark/heap (nếu task đã chạy)
- [ ] Không sửa main/BSP/config chung ngoài phạm vi được leader giao
- [ ] Không commit output/IDE cá nhân
