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

UI trigger:
- Chuột phải trên scope mở context menu.
- Chọn `Tools` để vào tab command tools.
- Click tool để chạy và nhận kết quả popup.

## Built-in Command Tool

Tool hiện tại:
- `stats_mean_max_min` (`Mean/Max/Min`)

Output:
- Thống kê theo từng channel trong range hiện tại
- Giá trị: mean, max, min, số lượng sample `n`

Range/Channel source:
- Lấy `timeWindow` từ scope đang active
- Lấy `channelIds` từ scope đang active (nếu scope đang filter)

## Hướng dẫn thêm command tool mới

### Bước 1: Tạo class tool trong `src/tools/command/`

Tool phải implement `ICommandTool`:

```cpp
class MyCommandTool final : public ICommandTool {
public:
    using ResultCallback = std::function<void(const QString&, const QString&)>;
    explicit MyCommandTool(ResultCallback callback);

    QString id() const override;
    QString name() const override;
    QString category() const override;
    bool isApplicable(const ToolContext& ctx) const override;
    void run(IScopeDataAPI* api) override;
};
```

Nguyên tắc:

- `id()` phải unique, ổn định.
- `isApplicable()` dùng `ToolContext` để filter tool trong menu.
- `run()` lấy dữ liệu qua `api->context()` + `api->snapshot(req)`.

### Bước 2: Cài đặt logic trong `run()`

Flow tối thiểu:

1. Lấy `ToolContext ctx = api->context()`.
2. Tạo `SnapshotRequest` theo `ctx.tMin/tMax` và channel filter.
3. Đọc `ScopeSnapshot` và tính toán.
4. Trả kết quả qua callback (hoặc signal riêng nếu bạn mở rộng UI).

Mẫu tham chiếu:

- `src/tools/command/stats_command_tool.h`
- `src/tools/command/stats_command_tool.cpp`

### Bước 3: Đăng ký tool trong `ScopeController`

Trong constructor `ScopeController`, thêm:

```cpp
registerCommandTool(std::make_shared<MyCommandTool>(
    [this](const QString& title, const QString& body) {
        emit commandToolResultReady(title, body);
    }));
```

File: `src/scope_controller.cpp`.

### Bước 4: Cập nhật CMake

Thêm file mới vào `APP_SOURCES`:

- `src/tools/command/my_command_tool.h`
- `src/tools/command/my_command_tool.cpp`

File: `CMakeLists.txt`.

### Bước 5: Verify trên UI

1. Build app.
2. Right-click trên scope.
3. Vào tab `Tools`.
4. Kiểm tra tool mới xuất hiện đúng điều kiện `isApplicable`.
5. Chạy tool và kiểm tra popup output.

## Checklist cho command tool mới

- `id` unique và không đổi theo thời gian.
- Không đọc trực tiếp internal store/ring buffer, chỉ dùng `IScopeDataAPI`.
- Không block UI thread với compute nặng.
- Output text rõ ràng (unit, range, channel id).
- Cập nhật docs `docs/project/*` sau khi thêm tool.
