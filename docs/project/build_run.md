# Build and Run

## Environment

Giả định hiện tại:
- Qt 6.5+ (đang build với Qt 6.9.3)
- CMake + Ninja
- macOS toolchain

## Build

Từ root project:

```bash
cmake --build build/Qt_6_9_3_for_macOS-Debug -j4
```

## Run

Binary tạo ra:

- `build/Qt_6_9_3_for_macOS-Debug/appC41Scope.app/Contents/MacOS/appC41Scope`

## Runtime Defaults

Fake generator mặc định:
- sample rate: `400 Hz`
- channels: `4`
- batch push: `32 samples`

View mặc định:
- `timeWindow = 5.0s`
- `liveMode = true`
- `paused = false`

QML usage (simple):

```qml
ScopeWorkbench {
    anchors.fill: parent
    controller: scopeController
}
```

## Quick Verification Checklist

- App mở lên có waveform realtime.
- Bấm `Zoom +`, `Zoom -`, `Fit`, `Pan` thấy thay đổi view window.
- Bấm `Split` mở matrix 5x5.
- Chọn layout > 1x1, nhiều scope render đồng thời.
- Click từng scope, tool Zoom/Fit tác động đúng scope active.
- Right-click trên scope mở command menu.
- Chạy `Mean/Max/Min` và thấy popup kết quả theo từng channel.

## Git Ignore

Các mục ignore chính trong repo:
- `build/`
- `CMakeLists.txt.user`
