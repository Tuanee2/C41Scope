# Rendering — Realtime Scope (Qt Quick Scene Graph)

Tài liệu này mô tả pipeline vẽ realtime cho C41Scope.

**Mục tiêu**
- Render mượt ở 60fps (hoặc vsync) với số kênh lớn.
- UI thread không bị block.
- Không allocation trong hot render path.

**Nguyên tắc bắt buộc**
- Core rendering phải dùng **Qt Quick Scene Graph**.
- Vẽ trong `QQuickItem::updatePaintNode()` bằng **QSGGeometryNode** (hoặc node QSG tương đương).
- Tuyệt đối **không dùng** `QPainter`/`Canvas` cho core realtime scope.

---

## 1. Scene Graph Lifecycle (QQuickItem)

### 1.1 Update flow
- Ingest thread chỉ set dirty flags.
- UI thread trigger repaint bằng `QQuickItem::update()`.
- Qt gọi `updatePaintNode(QSGNode* old, UpdatePaintNodeData*)` trên UI/Render thread theo cơ chế Scene Graph.

### 1.2 Quy tắc về thread
- Không assume updatePaintNode chạy trên worker.
- Không lock mutex lâu trong updatePaintNode.
- Không gọi API có thể block (I/O, sleep, heavy compute).

---

## 2. Render Pipeline (High-level)

Mỗi lần `updatePaintNode()` chạy:

1. **Read View State**
   - timeWindow, liveMode, paused, zoom/pan offsets
   - selection/cursor state
   - visible channels list

2. **Acquire Snapshot**
   - Lấy `ScopeSnapshot` immutable từ data layer thông qua controller/API.
   - Snapshot có thể trả về **nhiều segments** (HistoryStore chunked) hoặc 1 segment (FrameStore).

3. **Decimation / Resampling** (BẮT BUỘC)
   - Map time range → pixel width.
   - Không vẽ full raw samples nếu samples >> pixels.
   - Default: **min/max envelope theo pixel column**.

4. **Build/Update Geometry**
   - Reuse geometry buffers.
   - Update vertex arrays trong `QSGGeometry`.
   - Update bounding rect / dirty flags.

5. **Return Root Node**
   - Trả về node đã update để Qt render.

---

## 3. Data Representation for Rendering

### 3.1 Coordinate mapping
- X axis: time → pixel
  - `x = (t - tMin) / (tMax - tMin) * width`
- Y axis: value → pixel
  - `y = (1 - normalize(value)) * height`

`normalize(value)` dựa vào y-range hiện tại hoặc auto-scale.

### 3.2 Snapshot & Segments
Snapshot trả về cho mỗi channel:
- `t0`, `dt`
- `segments[]`: mỗi segment có `ptr`, `count`, `firstIndex` (hoặc firstTime)

Renderer phải handle 1 hoặc nhiều segments:
- HistoryStore: data nằm trong nhiều block → nhiều segments
- FrameStore: thường 1 segment

**Rule**: Renderer không được giữ pointer lâu hơn vòng đời của snapshot.

---

## 4. Decimation Strategy (Default)

### 4.1 Vì sao bắt buộc
Nếu width=1200px mà mỗi channel có 1,000,000 samples thì vẽ polyline trực tiếp sẽ:
- tốn CPU/GPU
- tốn băng thông upload geometry
- drop FPS

### 4.2 Min/Max envelope theo pixel column
Ý tưởng:
- Với mỗi cột pixel `x` (0..width-1), xác định khoảng sample index tương ứng.
- Lấy `min` và `max` trong khoảng đó.
- Vẽ segment dọc (x, yMin) → (x, yMax).

**Đầu ra vertices**
- Tối đa ~ `2 * width` points / channel / frame.
- Có thể pack thành line list hoặc triangle strip (tuỳ node).

### 4.3 Khi nào không cần decimation
- Khi số samples trong view ≤ (k * width) (k~1..2)
- Lúc đó có thể vẽ polyline downsample nhẹ.

---

## 5. Node & Geometry Layout

### 5.1 Node tree gợi ý
- Root: `QSGNode`
  - GridNode (optional)
  - AxesNode (optional)
  - ChannelsGroup
    - ChannelNode[0..N-1] (QSGGeometryNode)
  - Overlays (cursor, selection)

### 5.2 Reuse strategy
- Mỗi `ChannelNode` giữ:
  - `QSGGeometry` pre-allocated đủ cho max vertices per frame
  - `QSGMaterial` (hoặc shared material)

**Rule**:
- Không tạo node mới mỗi frame.
- Chỉ tạo/bỏ node khi channel add/remove hoặc style thay đổi lớn.

---

## 6. Dirty Flags & Scheduling

### 6.1 Dirty flag sources
- Data dirty: ingest có dữ liệu mới.
- View dirty: zoom/pan/timeWindow thay đổi.
- Style dirty: màu/line width.

### 6.2 Update policy
- Khi data dirty: `ScopeView.update()` theo cadence render (timer/vsync).
- Không update theo từng sample.

---

## 7. Performance Rules (BẮT BUỘC)

- Không allocation trong updatePaintNode (no `new`, no container grow).
- Không xây dựng QString/formatting trong render loop.
- Không lock mutex lâu.
- Không copy nguyên khối dữ liệu lớn mỗi frame.
- Decimation bắt buộc khi samples >> pixels.

---

## 8. Future (không bắt buộc giai đoạn 1)

- LOD cache (precomputed envelopes) cho history rất dài.
- GPU compute / instancing cho nhiều kênh.
- Partial redraw theo region.

