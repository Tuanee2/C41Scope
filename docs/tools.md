# Tools System — C41Scope

Mục tiêu: thiết kế hệ thống tools theo hướng mở rộng, tách rõ UI tools và compute tools, hỗ trợ plugin cho nhóm command tools.

---

## 1. Tool Categories

### 1.1 View Tools (Tools liên quan view)
**Ví dụ**: zoom in/out, pan, fit view, split view (chia nhiều scope), toggle grid/legend, đổi time window, đổi channel visibility...

**Đặc điểm**
- Tác động chính lên `ScopeViewState` và `ScopeView`.
- Không cần history sâu.
- Thường là built-in (core library).

**UI**
- Toolbar / shortcut / menu (tuỳ app).
- Không cần panel riêng.

---

### 1.2 Interactive Tools (Tools hiển thị panel bên phải)
**Ví dụ**: đo đạc trực tiếp (cursors, Δt, peak hold), search tương tác, marker editor, ROI/selection tools, statistics “live” theo vùng chọn.

**Đặc điểm**
- Cần vừa nhìn scope vừa thao tác → có UI panel bên phải.
- Có thể subscribe các trạng thái:
  - selection/cursor/time range
  - visible channels
- Tính toán nên chạy async nếu nặng.

**UI**
- Khi bật tool → panel xuất hiện bên phải scope.
- Tool có thể vẽ overlay/annotations (optional).

---

### 1.3 Command Tools (Tools mở từ chuột phải → dialog)
**Ví dụ**: FFT, histogram, export data, report generator, batch search, detect event, compute KPI… (không cần nhìn scope liên tục)

**Đặc điểm**
- Trigger từ context menu trên scope.
- Không cần panel dính với scope.
- Output dạng dialog / report / file export.
- Đây là nhóm **ưu tiên plugin**.

---

## 2. Tool Registry

`ScopeController` quản lý registry:

- `registerViewTool(tool)`
- `registerInteractiveTool(tool)`
- `registerCommandTool(tool)`

Registry cung cấp:
- danh sách tools để app dựng toolbar/right-panel/menu
- filter tools theo `Context` (mode History/Frame, selection có/không, channel count…)

---

## 3. Tool Context

Mọi tool nhận một context “chuẩn hóa” để tự quyết định applicability:

- `dataMode`: History | Frame
- `hasSelection`: bool
- `visibleChannels`: list
- `timeRange`: [tMin, tMax]
- `cursorState`: positions
- `scopeId` (nếu split view)

Context này phải lấy từ `IScopeDataAPI` (hoặc controller) và **không** cho tool chạm trực tiếp internal buffers.

---

## 4. Lifecycle (gợi ý)

### 4.1 View Tool
- Trigger → update view state → `ScopeView.update()`.

### 4.2 Interactive Tool
- Enable → create/show right panel
- Subscribe selection/cursor changes
- On disable → destroy/hide panel
- Compute nặng → chạy async (không block UI)

### 4.3 Command Tool
- User right-click scope → open menu → choose tool
- Tool mở dialog và pull snapshot/selection qua `IScopeDataAPI`
- Compute → show result in dialog/report
- Dialog close → tool kết thúc (stateless)

---

## 5. Rules (BẮT BUỘC)

- Tool không được emit signal high-frequency để vẽ.
- Tool không được lock mutex trong UI render loop.
- Tool/plugin không truy cập ringbuffer trực tiếp.
- Mọi tính toán phải dựa trên `ScopeSnapshot` immutable (read-only).
- Tool compute nặng phải async hoặc batch, không block UI thread.