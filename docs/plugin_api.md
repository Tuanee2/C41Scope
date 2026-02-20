# API — C41Scope Core Interfaces

Tài liệu này định nghĩa **API contract** giữa:
- Data ingest (worker thread)
- Rendering layer (QQuickItem / Scene Graph)
- Tools & Plugin system

Đây là **nguồn sự thật duy nhất** để Codex sinh code. Không được tự ý đổi tên class/hàm nếu chưa cập nhật tài liệu này.

---

## 1. Core Components Overview

Các thành phần chính:

- `ScopeView` — QQuickItem (UI/render node)
- `ScopeController` — lớp trung gian điều phối
- `IScopeDataStore` — interface dữ liệu (History / Frame)
- `IScopeDataAPI` — sandbox API cho tools/plugins
- `ScopeSnapshot` — dữ liệu immutable cho render/tools

Luồng dữ liệu:

```
Worker Thread → ScopeController → IScopeDataStore → ScopeSnapshot → ScopeView
Tools/Plugins → IScopeDataAPI → ScopeSnapshot
```

---

## 2. ScopeController (Public API)

`ScopeController` là entry point cho ingest và query snapshot. Lớp này được expose vào QML.

### 2.1 Data Mode

```cpp
enum class DataMode { History, Frame };
```

- `History`: append/chunked store (phình ra, không overwrite ở giai đoạn 1)
- `Frame`: replace frame (double-buffer)

### 2.2 Ingest API (thread-safe)

```cpp
class ScopeController : public QObject
{
    Q_OBJECT
public:
    void setDataMode(DataMode mode);
    DataMode dataMode() const;

    // History ingest (append)
    void pushSamples(int channelId,
                     const float* data,
                     size_t count,
                     double t0,
                     double dt);

    // Frame ingest (replace)
    void pushFrame(int channelId,
                   const float* data,
                   size_t count,
                   double t0,
                   double dt);
};
```

**Rules**
- Không emit signal high-frequency lên QML để vẽ.
- Không block lâu trong ingest.

---

## 3. Snapshot Request & Selection

### 3.1 Channel selection (scale tốt với nhiều kênh)

```cpp
struct ChannelSelection
{
    bool all = true;
    std::vector<int> channelIds;
};
```

### 3.2 SnapshotRequest

```cpp
struct SnapshotRequest
{
    double tMin;
    double tMax;

    ChannelSelection channels;

    int resolutionHint = 0;
};
```

---

## 4. ScopeSnapshot (Immutable View)

```cpp
struct ScopeSegment
{
    const float* data = nullptr;
    size_t       count = 0;
    double       t0 = 0.0;
    double       dt = 0.0;
};

struct ChannelSnapshot
{
    int channelId = -1;
    std::vector<ScopeSegment> segments;
};

struct ScopeSnapshot
{
    double tMin = 0.0;
    double tMax = 0.0;
    std::vector<ChannelSnapshot> channels;
};
```

**Lifetime rules (BẮT BUỘC)**
- Snapshot immutable/read-only.
- Renderer không được cache pointer qua nhiều frame.
- Tool async phải copy dữ liệu cần thiết.

---

## 5. IScopeDataStore Interface

```cpp
class IScopeDataStore
{
public:
    virtual ~IScopeDataStore() = default;

    virtual void pushSamples(int channelId,
                             const float* data,
                             size_t count,
                             double t0,
                             double dt) = 0;

    virtual void pushFrame(int channelId,
                           const float* data,
                           size_t count,
                           double t0,
                           double dt) = 0;

    virtual ScopeSnapshot buildSnapshot(const SnapshotRequest& req) = 0;

    virtual void clear() = 0;
};
```

Implementations:
- `HistoryStore` — append/chunked store
- `FrameStore` — double-buffer

---

## 6. IScopeDataAPI (Sandbox API cho Tools/Plugins)

```cpp
struct ToolContext
{
    DataMode dataMode;
    bool hasSelection;
    double tMin;
    double tMax;
};

class IScopeDataAPI
{
public:
    virtual ~IScopeDataAPI() = default;

    virtual ScopeSnapshot snapshot(const SnapshotRequest& req) = 0;

    virtual ToolContext context() const = 0;
};
```

---

## 7. ScopeView (QQuickItem API)

```cpp
Q_PROPERTY(QObject* controller READ controller WRITE setController NOTIFY controllerChanged)
Q_PROPERTY(double timeWindow READ timeWindow WRITE setTimeWindow NOTIFY timeWindowChanged)
Q_PROPERTY(bool   paused     READ paused     WRITE setPaused     NOTIFY pausedChanged)
```

```cpp
class ScopeView : public QQuickItem
{
public:
    void setController(QObject* ctrl);
};
```

---

## 8. Tools API (Core Level)

### 8.1 View Tools
```cpp
class IViewTool
{
public:
    virtual ~IViewTool() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual void trigger() = 0;
};
```

### 8.2 Interactive Tools
```cpp
class IInteractiveTool
{
public:
    virtual ~IInteractiveTool() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QUrl panelQml() const = 0;
    virtual void onActivate(IScopeDataAPI* api) = 0;
    virtual void onDeactivate() = 0;
};
```

### 8.3 Command Tools (Plugin)
```cpp
class ICommandTool
{
public:
    virtual ~ICommandTool() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString category() const = 0;
    virtual bool isApplicable(const ToolContext& ctx) const = 0;
    virtual void run(IScopeDataAPI* api) = 0;
};
```

---

## 9. Naming & Stability Rules (CHO AGENTS/CODEX)

- Không đổi tên ScopeController, ScopeView, ScopeSnapshot.
- Renderer chỉ đọc snapshot.
# Plugin API — C41Scope (Command Tools)

Mục tiêu: cho phép người dùng bổ sung Command Tools dưới dạng plugin nhưng bị sandbox bởi API giới hạn.

---

## 1. Plugin Model

- Plugin dạng C++ Qt Plugin (.dylib/.so/.dll)
- Load bằng QPluginLoader

---

## 2. Sandbox API (IScopeDataAPI)

Plugin tools chỉ được thao tác thông qua IScopeDataAPI.

Cung cấp:
- ScopeSnapshot snapshot(const SnapshotRequest& req)
- ToolContext context() const

Snapshot immutable/read-only. HistoryStore hiện tại là append/chunked nên snapshot có thể trả nhiều segments.

---

## 3. ICommandTool

```cpp
class ICommandTool
{
public:
    virtual ~ICommandTool() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString category() const = 0;
    virtual bool isApplicable(const ToolContext& ctx) const = 0;
    virtual void run(IScopeDataAPI* api) = 0;
};
```

---

## 4. IScopeToolPlugin

Plugin expose danh sách tools:

- QList<ICommandTool*> commandTools()

---

## 5. Security Rules

- Plugin chỉ dùng IScopeDataAPI.
- Không expose internal store.
- Không can thiệp threading model.