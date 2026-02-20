# Command Tools

## Folder Structure

Tool code được gom riêng để dễ mở rộng:

- `src/tools/view/`
  - `callback_view_tool.*`
- `src/tools/command/`
  - `stats_command_tool.*`

Mục tiêu:
- Tách biệt tool logic khỏi controller/view
- Dễ thêm tool mới cho nhà phát triển

## Runtime Wiring

`ScopeController` giữ registry và expose QML API:

- `viewToolItems`
- `commandToolItems`
- `triggerViewTool(toolId)`
- `triggerCommandTool(toolId)`

Signal kết quả command tool:
- `commandToolResultReady(title, body)`

## Built-in Command Tool

Tool hiện tại:
- `stats_mean_max_min` (`Mean/Max/Min`)

Output:
- Thống kê theo từng channel trong range hiện tại
- Giá trị: mean, max, min, số lượng sample `n`

Range/Channel source:
- Lấy `timeWindow` từ scope đang active
- Lấy `channelIds` từ scope đang active (nếu scope đang filter)

## How to Add New Command Tool

1. Tạo class mới trong `src/tools/command/` implement `ICommandTool`.
2. Đăng ký tool trong `ScopeController` constructor bằng `registerCommandTool(...)`.
3. Nếu cần output UI, dùng callback emit `commandToolResultReady(...)`.
4. Build + cập nhật docs `docs/project/*`.
