# AGENTS.md — AI Control Panel (Realtime Scope QML)

> **Entry point cho Codex/agent.** AI PHẢI đọc file này TRƯỚC KHI đọc bất kỳ thứ gì khác trong repo.

## Project Overview

- **Tên**: C41Scope
- **Mục tiêu**: Xây thư viện **realtime scope** cho **Qt Quick / QML** dựa trên **QQuickItem** để vẽ nhiều kênh dữ liệu theo thời gian thực (ví dụ: 400Hz, hàng trăm–nghìn kênh) với lịch sử, zoom/pan, cursor.
- **Ngôn ngữ tài liệu**: Tiếng Việt (code + API dùng tiếng Anh).

## Thứ tự đọc bắt buộc (Reading Order)

1. `AGENTS.md` (file này) — rules/constraints/workflow
2. `docs/_index.md` — bản đồ tài liệu (AI phải follow để chỉ load đúng file cần thiết)
3. Các file docs được `_index.md` trỏ tới (architecture/rendering/threading/perf/api)
4. `src/` (source code) — **code là source of truth**

> **Nguyên tắc**: Khi document và code mâu thuẫn → tin code.

## Rules (BẮT BUỘC)

### 1) Explicit > Implicit
- Không đoán. Nếu requirement mơ hồ → nêu giả định rõ ràng trong PR/commit message hoặc hỏi lại.
- Không tự ý thêm feature ngoài scope task.

### 2) Realtime + Qt Quick constraints
- **Tuyệt đối không** emit signal lên QML ở tần số cao (ví dụ 400Hz) để trigger vẽ.
- UI thread **không được block** vì ingest dữ liệu.
- Không dùng `QPainter`/`Canvas` cho core realtime rendering. Rendering core phải theo **Scene Graph**.

### 3) Rendering rules (Qt Quick Scene Graph)
- Item hiển thị scope phải kế thừa `QQuickItem`.
- Vẽ trong `updatePaintNode()` bằng `QSGGeometryNode` (hoặc kiến trúc QSG tương đương).
- Mỗi frame **không được cấp phát động** (no new/delete, no QVector grow, no QString build trong hot-path).
- Bắt buộc có **decimation** theo pixel-width (ví dụ min/max envelope) để không vẽ quá nhiều điểm vô nghĩa.

### 4) Threading rules
- Ingest dữ liệu có thể đến từ thread khác.
- Không lock mutex “to” trong render loop. Nếu cần đồng bộ:
  - ưu tiên **double-buffer** / **atomic flags** / **lock-free queue**.
  - nếu buộc dùng lock → dùng `try_lock` + fallback bỏ frame, không chặn UI.

### 5) Memory rules
- Ring buffer / history buffer phải **pre-allocate** theo cấu hình.
- Không copy nguyên khối dữ liệu lớn giữa thread nếu không cần; ưu tiên batch + pointer/span.
- Tránh lưu per-sample timestamp dạng double nếu có thể (ưu tiên index + t0 + dt).

### 6) API rules
- API công khai phải ổn định, tối giản, dễ dùng từ QML.
- Phần điều khiển (controller) là `QObject` với `Q_PROPERTY` cần thiết; tránh expose cấu trúc phức tạp sang QML.
- Không tạo dependency nặng/ngoài Qt (không thêm thư viện ngoài) trừ khi task yêu cầu.

### 7) Coding style
- C++: modern C++ (C++17 trở lên), rõ ràng, ít macro.
- Tên lớp: `PascalCase`, hàm/biến: `camelCase` hoặc `lower_snake_case` nhưng **nhất quán** trong repo.
- Comment giải thích **WHY**, không giải thích WHAT.

### 8) Build / tests
- Mọi thay đổi phải giữ build chạy.
- Khi thêm feature lớn: kèm demo minimal + test tối thiểu (unit test cho ring buffer/decimation nếu phù hợp).

## Workflow bắt buộc cho agent (Codex-friendly)

### A) Trước khi code
1. Đọc `docs/_index.md` để chọn đúng file docs cần đọc.
2. Tóm tắt lại requirement + quyết định kiến trúc (ngắn gọn) trong PR description hoặc commit body.

### B) Khi code
- Đi từ skeleton → chạy được demo → tối ưu.
- Ưu tiên correctness + architecture đúng rules trước, rồi mới micro-optimizations.

### C) Khi thiếu thông tin
- Nếu thiếu các thông số quan trọng (sample rate, history seconds, max channels, format ingest):
  - đưa ra default hợp lý **và ghi rõ trong docs/commit**.

## Default assumptions (nếu user chưa cung cấp)

- Target: Qt 5.12+.
- Refresh: 60fps (vsync/update timer), ingest tần số cao từ worker.
- Decimation: min/max envelope theo pixel column.

## Deliverables checklist

- [ ] `ScopeView` (QQuickItem) render qua QSG
- [ ] `ScopeController` (QObject) API điều khiển + QML-facing
- [ ] Thread-safe ingest + ring buffer history
- [ ] Decimation theo pixel width
- [ ] Demo app QML + fake generator
- [ ] Docs cập nhật (docs/*) khi thay đổi kiến trúc/API
- [ ] Project document (doc*/project/*) được cập nhật mỗi lần sau mỗi lần thay đổi và build thành công