# Current Implementation

## Scope

Project hiện tại là skeleton realtime scope cho Qt Quick/QML, tập trung vào:

- Render bằng QSG (`QQuickItem::updatePaintNode`)
- Ingest data từ worker thread
- Snapshot immutable cho render
- View tools cơ bản (Zoom, Fit, Pan, Split)
- Command tools chạy từ context menu chuột phải

## Component Map

### 1. `ScopeController`

File:
- `src/scope_controller.h`
- `src/scope_controller.cpp`

Trách nhiệm:
- Public API ingest: `pushSamples`, `pushFrame`
- Chuyển mode `History` / `Frame`
- Bridge data API cho render/tools: `snapshot`, `context`
- Registry cho view tools + command tools
- Expose QML models: `viewToolItems`, `commandToolItems`
- Expose `channelIds` để UI split phân bổ channel
- Phát `splitToolRequested` khi user chọn tool `Split`
- Phát `commandToolResultReady` khi command tool có output
- `context()` ưu tiên dùng `timeWindow` + `channelIds` của scope đang active để command tool lấy đúng range/channel hiển thị

Concurrency:
- Dùng atomic cho time range, dirty state, data epoch
- `dataEpoch` tăng mỗi lần ingest/clear/mode switch để nhiều `ScopeView` cùng phát hiện data mới

### 2. Data Store Layer

Files:
- `src/store/history_store.*`
- `src/store/frame_store.*`

`HistoryStore`:
- Append dữ liệu theo chunk per-channel
- `buildSnapshot` dùng `try_lock`; nếu không lock được trả snapshot gần nhất
- Snapshot có thể chứa nhiều segment

`FrameStore`:
- Frame replace theo channel
- `buildSnapshot` copy sang `m_snapshotBuffers` để giữ pointer ổn định cho vòng đời snapshot
- `try_lock` fallback snapshot gần nhất

### 3. `ScopeView`

Files:
- `src/scope_view.h`
- `src/scope_view.cpp`

Trách nhiệm:
- QML-facing view state: `timeWindow`, `paused`, `liveMode`, `showGrid`, `channelIds`
- Render QSG bằng `QSGGeometryNode`
- Decimation min/max theo pixel column
- Update cadence theo timer 60fps
- Mỗi view tự theo dõi `dataEpoch` để update độc lập (hỗ trợ split nhiều scope)

View actions:
- `zoomIn`, `zoomOut`, `fitView`, `pan`, `panByPixels`

History modes:
- Auto-follow mode (`liveMode=true`): cửa sổ bám dữ liệu mới.
- Manual mode (`liveMode=false`): cửa sổ đứng yên để user duyệt history.
- `Pan` chuyển scope active sang manual mode; sau đó kéo chuột trái để pan theo thời gian.

`pan` behavior:
- Chuyển scope sang manual mode.
- Pan thực tế được thực hiện qua kéo chuột (`panByPixels`) trong UI layer.

Decimation:
- Input: `ScopeSnapshot` segments
- Output: 2 vertices/pixel-column (envelope line)

### 4. Tools Source Organization

Files:
- `src/tools/view/callback_view_tool.*`
- `src/tools/command/stats_command_tool.*`

Hiện tại:
- View tools: Zoom+/Zoom-/Fit/Pan/Split
- Command tool: Mean/Max/Min (thống kê theo channel)

### 5. Demo Ingest

Files:
- `src/demo/fake_data_generator.*`
- `main.cpp`

Trách nhiệm:
- Worker thread tạo tín hiệu giả dạng sin/wobble
- Default: 400Hz, 4 channels, batch 32
- Push vào `ScopeController`

### 6. QML UI Packaging

Files:
- `Main.qml`
- `qml/scopeui/ScopeWorkbench.qml`
- `qml/scopeui/ScopeSplitGrid.qml`
- `qml/scopeui/ScopeToolbar.qml`
- `qml/scopeui/ScopeSplitPicker.qml`
- `qml/scopeui/ScopeCommandMenu.qml`
- `qml/scopeui/ScopeCommandResultDialog.qml`

Thiết kế:
- `Main.qml` giữ tối giản, chỉ mount một component `ScopeWorkbench`.
- Logic UI scope được tách vào `qml/scopeui/` để dễ đọc và mở rộng.
- Dev dùng scope trong QML theo kiểu component-level, không cần giữ toàn bộ logic trong một file lớn.
- `ScopeWorkbench` giữ state hiển thị menu/popup (`commandMenuVisible`, `splitPickerVisible`, `commandResultVisible`) và đóng/mở qua signal từ component con để tránh phá vỡ property binding.
- Toolbar hiển thị trạng thái scope active (`Follow` hoặc `Manual`) cùng `timeWindow`.

## API/Docs Alignment Notes

- Đã có `IScopeDataStore`, `IScopeDataAPI`, `ScopeSnapshot` theo docs core.
- Tooling runtime đã có view + command tools ở mức built-in.
- Plugin/interactive tools chưa được implement đầy đủ (mới ở mức interface).
