# Library Usage Guide

Tài liệu này hướng dẫn cách dùng C41Scope ở mức tích hợp ứng dụng: khởi tạo controller, mount QML scope, ingest dữ liệu realtime, và dùng tool menu.

## 1. Khởi tạo trong C++

Tối thiểu cần:

1. Đăng ký `ScopeView` cho QML.
2. Tạo `ScopeController`.
3. Đưa controller vào QML context.
4. Load QML module.

Ví dụ theo code hiện tại:

```cpp
QGuiApplication app(argc, argv);

qmlRegisterType<c41scope::ScopeView>("C41Scope", 1, 0, "ScopeView");

c41scope::ScopeController scopeController;

QQmlApplicationEngine engine;
engine.rootContext()->setContextProperty("scopeController", &scopeController);
engine.loadFromModule("C41Scope", "Main");
```

File tham chiếu: `main.cpp`.

## 2. Gắn scope vào QML

Có 2 cách:

1. Dùng `ScopeWorkbench` (khuyến nghị)
- Đã có toolbar view tools, split picker, menu chuột phải và command tools popup.

```qml
ScopeWorkbench {
    anchors.fill: parent
    controller: scopeController
}
```

2. Dùng trực tiếp `ScopeView`
- Phù hợp khi bạn tự xây UI riêng.

```qml
ScopeView {
    anchors.fill: parent
    controller: scopeController
    timeWindow: 5.0
    liveMode: true
    paused: false
}
```

## 3. Ingest dữ liệu realtime

Controller có 2 API ingest thread-safe:

- `pushSamples(...)`: cho `History` mode (append history)
- `pushFrame(...)`: cho `Frame` mode (replace frame hiện tại)

Yêu cầu dữ liệu:

- `channelId`: id kênh
- `data`: pointer float samples
- `count`: số mẫu
- `t0`: timestamp mẫu đầu
- `dt`: sample interval (giây)

Khuyến nghị:

- Gọi ingest từ worker thread.
- Không emit signal QML ở tần số cao để trigger vẽ.
- Batch samples thay vì gọi từng sample.

## 4. Chọn mode dữ liệu

`ScopeController` có property QML:

- `dataMode = 0` → `History`
- `dataMode = 1` → `Frame`

Khi đổi mode, store sẽ reset theo implementation hiện tại.

## 5. History follow và pan

Mỗi `ScopeView` có 2 mode:

- `liveMode=true`: auto-follow dữ liệu mới (mặc định)
- `liveMode=false`: manual history mode để duyệt lại dữ liệu cũ

Trong UI hiện tại:

- Bấm `Pan` sẽ chuyển scope active sang manual mode.
- Kéo chuột trái để pan thời gian.
- Chuột phải bật lại `History Follow` để quay về auto-follow.

## 6. Split và active scope

- `Split` mở ma trận 5x5 để chọn số hàng/cột.
- Sau khi split, channel được phân bổ round-robin theo cell.
- Click vào scope con để chọn active scope (`viewToolTarget`), view tool sẽ tác động lên scope này.

## 7. Kiểm tra nhanh sau tích hợp

1. Mở app thấy waveform realtime.
2. Bấm `Zoom +`, `Zoom -`, `Fit`, `Pan` thấy thay đổi đúng scope active.
3. Bấm `Split` chọn layout > 1x1, nhiều scope cùng cập nhật dữ liệu.
4. Chuột phải thấy `History Follow` và tab `Tools`.
5. Chạy command tool và nhận popup kết quả.
