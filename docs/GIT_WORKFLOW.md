# Quy trình Git theo yêu cầu thầy

## Nhánh

- `main`: chỉ bản ổn định đã chạy trên kit; cấm commit trực tiếp sau lần bootstrap ban đầu.
- `develop`: nhánh tích hợp; mọi `feature/...` tạo từ đây và PR trở lại đây.
- `feature/<ten-task>`: từng phần việc của từng người.
- `integration/system-v1`: leader ghép và kiểm toàn hệ thống trước release.

File ZIP chỉ chứa khung. Chưa tạo branch/protection hay đẩy lên repo GitHub.
Repo nhóm trong đề cương: https://github.com/die666web/BTL_Nhom14.

## Leader đưa khung lên repo

Repo đã tồn tại: tạo nhánh `feature/project-scaffold` từ develop, đưa file vào, kiểm diff,
PR về develop; không dùng force push đè lịch sử và không chạy git init lại.
Repo mới hoàn toàn: lần commit khung đầu lên main là ngoại lệ bootstrap trong tài liệu thầy,
sau đó tạo develop; từ đó bật bảo vệ main và chỉ merge qua PR.
Bản khung chưa kiểm kit không được gọi là release stable/v1.0.0.

## Thành viên

```bash
git switch develop
git pull --ff-only origin develop
git switch -c feature/task-display
# Chỉ sửa file của mình + test/tài liệu liên quan
git add app/src/task_display.c tests/manual/CONG.md
git commit -m "feat(task-display): render measurement mailbox on OLED"
git push -u origin feature/task-display
```

Mở PR **feature/task-display -> develop**. Người khác review API, timeout, mutex/ISR,
không warning, test kit/stack; leader merge từng PR. Người viết task không sửa main.

Đồng bộ nhánh riêng:

```bash
git fetch origin
git rebase origin/develop
# Resolve conflict, git add <file>, git rebase --continue
# Chỉ dùng với nhánh cá nhân đã thống nhất, không với develop/main:
git push --force-with-lease origin feature/task-display
```

## Integration và release

Leader tạo `integration/system-v1` từ develop. Bật task, ghép main/IPC, test dữ liệu,
ưu tiên/stack/heap/deadlock và lỗi. Khi đạt: merge integration -> develop, develop -> main,
đánh tag `v1.0.0`; xóa integration khi hoàn tất. Không release trước test phần cứng.

## Kỷ luật chung Linux/Keil

- Chỉ Mạnh sửa pin/clock/FreeRTOS/build hoặc sinh code; không generate CubeMX/RTE đè startup chung.
- Dùng `...FromISR()` trong ISR; task delay dùng RTOS; mọi chờ thiết bị/queue đều có timeout.
- Commit: `feat(task-xxx): ...`, `fix(rtos): ...`, `refactor(...): ...`.
- Không commit build, Debug, Release, binary, IDE settings cá nhân.
- Khác biệt xuống dòng được `.gitattributes` xử lý; không reformat toàn bộ file người khác.
- Protection main/develop, required reviews/status checks phải đặt trên GitHub bởi người có quyền;
  README hay `.gitignore` không tự chặn push.
