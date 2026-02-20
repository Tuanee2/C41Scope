
# C41Scope Documentation Index

> Entry point cho toàn bộ tài liệu dự án **Realtime Scope QML (Qt Quick / QQuickItem)**.
> AI agent (Codex) phải đọc file này trước để quyết định load doc nào tiếp theo.

---

## 📐 Core Architecture

- [architecture.md](architecture.md)
  - Kiến trúc tổng thể (ScopeView/Controller/DataStore/Tools/Plugins)
  - Data flow & ownership boundaries
  - Invariants (realtime/perf/threading)

---

## 🎛 Tools System

- [tools.md](tools.md)
  - Phân loại tools (View / Interactive Panel / Command Dialog)
  - Lifecycle & UI placement
  - Tool registry và context menu

- [plugin_api.md](plugin_api.md)
  - Plugin interface (QPluginLoader)
  - Sandbox API cho tools (IScopeDataAPI)
  - Versioning/compatibility rules

---

## 🎨 Rendering Pipeline (Realtime Scene Graph)

- [rendering.md](rendering.md)
  - Lifecycle của QQuickItem
  - updatePaintNode workflow
  - QSGGeometryNode usage
  - Decimation strategy (min/max envelope)

---

## 🧵 Threading Model

- [threading.md](threading.md)
  - Worker thread ingest dữ liệu
  - UI thread rendering
  - Double-buffer / atomic sync rules

---

## 🔌 Public API

- [api.md](api.md)
  - ScopeController API (QML-facing)
  - Channel management
  - Data ingestion interface
  - QML exposed properties

---

## 📁 Project Docs (Current State)

- [project/_index.md](project/_index.md)
  - Tài liệu chi tiết theo trạng thái code hiện tại
  - Bao gồm implementation map, split tool UI, build/run
  - Bao gồm guide tích hợp/sử dụng lib và quy trình thêm command tool

---

## 🧠 Reading Strategy for Agents

**Chỉ load đúng file cần thiết**, tránh lãng phí context window.

1.	Luôn đọc architecture.md trước để nắm cấu trúc tổng thể.
2.	Luôn đọc api.md trước khi sinh code hoặc đổi interface.
3.	Nếu task liên quan UI/interaction/tooling → đọc tools.md.
4.	Nếu task liên quan plugin → đọc plugin_api.md.
5.	Nếu task liên quan vẽ realtime → đọc rendering.md.
6.	Nếu task liên quan ingest/sync → đọc threading.md.
7.	Nếu task cần trạng thái triển khai thực tế hiện tại → đọc `project/_index.md`.

---

## ✅ Project Constraints (Summary)

- Rendering phải dùng **Qt Quick Scene Graph** (updatePaintNode + QSG).
- Không dùng **QPainter/Canvas** cho core realtime scope.
- Không allocation trong mỗi frame render.
- Không emit signal tần số cao lên QML để vẽ.
- Ưu tiên performance và realtime stability.
