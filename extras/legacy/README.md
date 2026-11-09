# Thư viện cũ giữ tham khảo

SPI1, TIM2 PWM và MAX7219 từ ZIP gốc; chỉ đổi include theo vị trí mới.
Không nằm trong source manifest/build Keil hoặc GCC của đề tài hiện tại.
Không giao thành viên viết lại những phần đã có.

Chưa phù hợp để gọi chung từ task: SPI polling chưa timeout/mutex; MAX7219 cần lock toàn
CS transaction nếu nhiều task; PWM TIM2 channel 1 chiếm PA0 đang là nút nhấn.
MAX7219 không điều khiển OLED I2C. Relay chỉ ON/OFF, không dùng timer PWM cho relay cơ.
