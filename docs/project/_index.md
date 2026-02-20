# Project Docs Index

Tài liệu chi tiết trạng thái triển khai thực tế của project (`code là source of truth`).

## Files

- [implementation.md](implementation.md)
  - Kiến trúc hiện tại theo code trong `src/`
  - Mapping class chính và data flow ingest/render

- [library_usage.md](library_usage.md)
  - Hướng dẫn tích hợp và sử dụng lib trong C++/QML
  - Ingest, mode History/Frame, follow/manual, split và kiểm tra nhanh

- [ui_split_tools.md](ui_split_tools.md)
  - Menu View Tools góc trái trên
  - Tool Zoom/Fit/Split và matrix split 5x5
  - Quy tắc chia channel từ scope gốc sang scope con

- [command_tools.md](command_tools.md)
  - Tổ chức source tool theo folder riêng
  - Command tools registry, right-click menu, output dialog
  - Quy trình thêm command tool mới (class, controller wiring, CMake, verify)

- [build_run.md](build_run.md)
  - Cách build, chạy demo, và kiểm tra nhanh
  - Giả định runtime hiện tại (sample rate, channel count)

## Update Rule

Mỗi lần thay đổi code và build thành công phải cập nhật lại ít nhất một file trong `docs/project/` để phản ánh trạng thái mới.
