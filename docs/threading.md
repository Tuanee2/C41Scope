

# Threading — Ingest vs UI Rendering

Tài liệu này mô tả mô hình đa luồng cho C41Scope.

**Mục tiêu**
- Ingest dữ liệu tần số cao từ worker thread.
- UI thread/render loop không bị block.
- Không emit signal tần số cao lên QML.

---

## 1. Threads

### 1.1 UI Thread
- QML engine
- input events
- `QQuickItem::update()`
- `updatePaintNode()` và build geometry

### 1.2 Worker Thread(s)
- Nhận dữ liệu realtime (network/serial/IPC)
- Gọi API ingest: `ScopeController::push...` (thread-safe)

---

## 2. Golden Rules (BẮT BUỘC)

1. **Không emit high-frequency signals lên QML để vẽ**.
2. **Không block UI thread** (no mutex lâu, no I/O, no sleep).
3. Render loop không được phụ thuộc vào tốc độ ingest.
4. Đồng bộ ưu tiên: atomic publish / double-buffer.

---

## 3. Data Stores & Synchronization

C41Scope có 2 mode data store (xem `architecture.md`):
- HistoryStore (temporary): append/chunked store (phình ra)
- FrameStore: replace frame (double-buffer)

### 3.1 FrameStore (FrameScope) — double-buffer swap
**Ý tưởng**
- Có 2 buffer: front (UI đọc), back (worker ghi).
- Worker ghi xong → swap atomically.

**Publish**
- Worker:
  - fill `back`
  - `publishSeq++` (store-release)
  - swap pointer/index (atomic)
- UI:
  - load pointer/index (acquire)
  - đọc snapshot consistent

**Ưu điểm**
- Snapshot luôn consistent.
- Chi phí thấp, dễ reasoning.

### 3.2 HistoryStore (HistoryScope) — append/chunked + snapshot publish
**Ý tưởng**
- Dữ liệu lưu theo các block fixed-size (ví dụ 1–5s/block).
- Worker append block hoặc append vào block đang mở.
- UI đọc theo time range; snapshot có thể gồm nhiều segments.

**Publish model (khuyến nghị)**
- Mỗi channel có `appendSeq` (atomic sequence) hoặc global `historySeq`.
- Worker:
  - append data vào block (không realloc trong hot path nếu pre-reserve block)
  - khi dữ liệu “commit” xong → `appendSeq.store(newSeq, release)`
- UI:
  - `seq = appendSeq.load(acquire)`
  - build `ScopeSnapshot` dựa trên seq

**Rule quan trọng**
- UI chỉ đọc những phần data đã được publish (acquire/release đảm bảo visibility).
- Tránh `std::vector` realloc trong lúc UI đang đọc.

> Gợi ý thực tế: dùng `std::deque<Block>` hoặc container không invalidate pointer khi push_back, và `Block` chứa buffer pre-allocated.

---

## 4. Snapshot Semantics

### 4.1 Snapshot là immutable
- Renderer/tools nhận snapshot chỉ đọc.
- Snapshot không cho phép mutate data store.

### 4.2 Multi-segment snapshot
- HistoryStore có thể trả về nhiều segment theo block để tránh copy.
- Tool/renderer phải handle 1..K segments.

### 4.3 Lifetime
- Snapshot chỉ valid trong vòng `updatePaintNode()` hoặc trong thời gian tool giữ snapshot theo hợp đồng.
- Nếu tool chạy async, nên **copy** dữ liệu cần thiết hoặc yêu cầu API cung cấp buffer ổn định theo job.

---

## 5. Scheduling: Khi nào gọi update()

### 5.1 Dirty flags
- Worker chỉ set `dataDirty=true` (atomic) khi có data mới.

### 5.2 UI refresh cadence
- UI refresh theo timer/vsync (ví dụ 60fps).
- Mỗi tick:
  - nếu `dataDirty` hoặc viewDirty → `ScopeView.update()`

**Không** được gọi update() theo từng sample.

---

## 6. Locking Policy

- Render loop: **no blocking lock**.
- Nếu bắt buộc có mutex (giai đoạn đầu):
  - dùng `try_lock`
  - nếu fail → bỏ frame và render snapshot cũ

---

## 7. Async compute for tools

- Tool compute nặng (FFT/search) không chạy trên UI thread.
- API cung cấp `requestAsyncJob(...)`:
  - core copy data cần thiết (small window) hoặc pin snapshot
  - chạy job trên worker pool
  - callback trả kết quả lên UI thread

---

## 8. Pitfalls (những lỗi hay gặp)

- Emit 400Hz lên QML → UI lag, drop frames.
- Realloc container khi UI đang đọc → crash/undefined behavior.
- Copy full history mỗi frame → tốn CPU/memory.
- Lock mutex trong updatePaintNode → giật.