# Architecture — C41Scope (Realtime Scope for QML)

Tài liệu này định nghĩa kiến trúc “xương sống” cho thư viện realtime scope chạy trên Qt Quick/QML.
Mục tiêu chính:

- Ingest dữ liệu tần số cao từ worker thread.
- Render 60fps (hoặc vsync) bằng **Scene Graph**.
- UI thread **không bị block**, không emit high-frequency signals.
- Hỗ trợ **2 loại scope**:
  - **HistoryScope**: lưu toàn bộ dữ liệu vào history.
  - **FrameScope**: không lưu history, frame mới sẽ thay thế hoàn toàn frame cũ.
- Có hệ thống **Tools** (View/Interactive/Command) và **Plugin** cho Command tools.

---

## 1. High-level Components

### 1.1 ScopeView (QQuickItem)
**Vai trò**: UI + render + input.

- QML-facing properties: `controller`, `timeWindow`, `paused`, `liveMode`, `showGrid`, ...
- Nhận input (mouse/trackpad): zoom/pan/cursor/selection.
- Render trong `updatePaintNode()` bằng QSG (xem `rendering.md`).

**Không làm**:
- Không ingest dữ liệu trực tiếp từ worker.
- Không giữ ring buffer/history.

### 1.2 ScopeController (QObject)
**Vai trò**: API công khai + điều phối.

- Expose vào QML: `ScopeView.controller: scopeController`.
- Quản lý channels (add/remove/enable/scale).
- Nhận dữ liệu từ data source (có thể gọi từ thread khác): `pushSamples` (History) / `pushFrame` (Frame).
- Quản lý mode dữ liệu: `dataMode = History | Frame`.
- Quản lý tool registry & plugin manager.

### 1.3 Data Layer: IScopeDataStore
**Vai trò**: ingest + lưu trữ + cấp snapshot cho render/tools.

Interface logic (khái niệm):
- `pushSamples(...) (History append)`
- `pushFrame(...) (Frame replace)`
- `buildSnapshot(request) → ScopeSnapshot`
- `clear()`

Có 2 implementation:

#### (A) HistoryStore (HistoryScope)
- **Giai đoạn hiện tại (temporary)**: lưu history theo kiểu **append (phình ra)** để đảm bảo có thể xem lại dữ liệu rất lâu (hàng giờ) mà **không bị overwrite**.
- Implementation gợi ý:
  - **Chunked store**: dữ liệu được lưu theo các block cố định (ví dụ 1–5 giây / block) và append vào `std::vector`/`std::deque` các block.
  - Mỗi channel có danh sách block riêng hoặc dùng layout interleaved theo frame (tuỳ ingest format).
- `snapshot(request)` sẽ đọc theo `timeRange` và có thể trả về **nhiều segments** (mỗi segment ứng với một block) để tránh copy.

> Lưu ý: cách lưu “phình ra” có rủi ro RAM lớn. Sau này sẽ thay bằng **2 tầng** (RAM cache + archive/LOD) hoặc chính sách giới hạn (drop/flush) nhưng **chưa làm ở giai đoạn này**.

#### (B) FrameStore (FrameScope)
- Không lưu lịch sử.
- Frame mới **replace** frame cũ (overwrite), snapshot chỉ phản ánh frame hiện tại.

> Renderer và Tools chỉ làm việc với `ScopeSnapshot` nên không cần biết store là History hay Frame.

### 1.4 Tool System
Tool system chia 3 nhóm (chi tiết ở `tools.md`):

- **View Tools**: zoom/pan/split layout... thao tác vào view state.
- **Interactive Tools (Right Panel)**: cần nhìn scope, hiển thị panel bên phải.
- **Command Tools (Dialog)**: mở từ context menu, không cần nhìn scope, chạy tính toán và hiển thị dialog.

### 1.5 PluginManager
**Vai trò**: load tool plugins.

- Scan thư mục `plugins/` và load qua `QPluginLoader`.
- Plugin cung cấp chủ yếu **Command Tools** (và có thể mở rộng thêm Interactive trong tương lai).
- Plugin chỉ thao tác thông qua sandbox API: `IScopeDataAPI` (xem `plugin_api.md`).

---

## 2. Data Flow

### 2.1 Ingest path (Worker thread)
Data source → `ScopeController::push...` → `IScopeDataStore::push...` → set `dirtyFlag`.

**Rule**:
- Không emit high-frequency signals lên QML.
- Ingest chỉ cập nhật data store + dirty flags (atomic).

### 2.2 Render path (UI thread)
`ScopeView` (timer/vsync) → `update()` → `updatePaintNode()`:

1. Query view state (timeWindow, offsets, zoom/pan, selection).
2. Pull `ScopeSnapshot` từ `ScopeController`/`IScopeDataAPI`.
3. Decimate theo pixel width (min/max envelope).
4. Update QSG geometry nodes (reuse memory).

### 2.3 Tool execution path
- View tool: trigger ngay, chỉnh view state, gọi `ScopeView.update()`.
- Interactive tool: panel bên phải subscribe selection/cursor; compute có thể async.
- Command tool: mở từ context menu, nhận snapshot/selection qua `IScopeDataAPI`, hiển thị dialog.

---

## 3. Ownership & Boundaries

### 3.1 Ownership
- `ScopeView` **không sở hữu** data store.
- `ScopeController` sở hữu/điều phối:
  - `IScopeDataStore` (HistoryStore/FrameStore)
  - `ToolRegistry`
  - `PluginManager`
- Tools chỉ giữ reference tới `IScopeDataAPI` (read-only + async requests).

### 3.2 Boundaries (sandbox)
- Plugin tools **không** được truy cập ring buffer/internal structs trực tiếp.
- Mọi đọc dữ liệu phải đi qua `IScopeDataAPI` và `ScopeSnapshot` (immutable).

---

## 4. Concurrency Model (tóm tắt)

- UI thread: render + input.
- Worker thread(s): ingest.

**Không lock mutex trong render loop**.

### 4.1 Đồng bộ khuyến nghị
- HistoryStore (temporary): append/chunked store + publish snapshot bằng atomic indices/sequence (UI đọc snapshot consistent, worker append không block UI).
- FrameStore: double-buffer swap pointer atomically.

Chi tiết: xem `threading.md`.

---

## 5. Scope Modes (History vs Frame)

### 5.1 HistoryScope
- Dữ liệu push lên **được lưu lại toàn bộ** trong history theo kiểu **append/chunked store** (không overwrite).
- View có thể pause và browse lại history (kể cả dữ liệu từ rất lâu trước đó, phụ thuộc RAM).

### 5.2 FrameScope
- Mỗi frame mới lên sẽ **xóa/replace** dữ liệu cũ.
- Snapshot luôn là dữ liệu hiện tại.

### 5.3 Switching mode
- `ScopeController::setDataMode(...)` sẽ:
  - tạo store mới tương ứng
  - giữ channel metadata (nếu phù hợp)
  - reset data (clear)

---

## 6. Invariants (BẮT BUỘC)

- Core rendering dùng **Qt Quick Scene Graph** (không QPainter/Canvas).
- Không allocation trong hot render path.
- Decimation bắt buộc theo pixel width.
- Ingest không block UI.
- Tools/plugin chỉ thao tác qua sandbox API.

---

## 7. File/Folder Layout

- `src/`: toàn bộ code
- `docs/`: tài liệu kiến trúc & spec
- `examples/`: demo apps
- `plugins/`: binary plugins (optional)

```text
C41Scope/
  CMakeLists.txt
  src/
  docs/
  examples/
  plugins/
```