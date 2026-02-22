# UI Tools (View + Command) and Split

## View Tool Menu

Vị trí:
- Trên một thanh công cụ riêng ở phía trên vùng scope (không nằm trong scope nào).
- Cụ thể: `ScopeWorkbench` render `toolStrip`, và `ScopeToolbar` nằm trong thanh này.

Nguồn dữ liệu menu:
- `scopeController.viewToolItems`

Trigger action:
- `scopeController.triggerViewTool(toolId)`

Tool hiện có:
- `zoom_in` (`Zoom +`)
- `zoom_out` (`Zoom -`)
- `fit` (`Fit`)
- `pan` (`Pan`)
- `split` (`Split`)

## History View Modes

Mỗi scope có 2 mode xem history:

1. Auto-follow mode
- Cửa sổ tự chạy theo dữ liệu mới nhất.
- Đây là mode mặc định.

2. Manual history mode
- Cửa sổ không tự chạy theo dữ liệu mới.
- Dùng để duyệt lại dữ liệu cũ.
- Có thể bật/tắt lại mode này từ context menu chuột phải (`History Follow`).

## Pan Tool Behavior

- `Pan` là view tool, tác động lên scope đang active (`viewToolTarget`).
- Khi bấm `Pan`:
  - scope chuyển sang manual history mode (`liveMode=false`).
- Sau đó user kéo chuột trái trên scope để pan theo trục thời gian.
- Nếu cần quay lại chạy theo dữ liệu mới, bật lại cờ `History Follow`.

Pan drag:
- Kéo trong manual mode sẽ dịch cửa sổ theo vị trí chuột.
- Không đổi `timeWindow`, chỉ đổi vùng thời gian đang xem.

## Split Tool Flow

1. User bấm `Split`.
2. `ScopeController` emit signal `splitToolRequested`.
3. QML mở popup matrix 5x5.
4. Khi hover matrix, preview số hàng/cột theo ô đang trỏ.
5. Khi click ô, apply `splitRows x splitCols`.

Component tách riêng:
- Grid scope: `qml/scopeui/ScopeSplitGrid.qml`
- Legend overlay: `qml/scopeui/ScopeLegend.qml`
- Split picker: `qml/scopeui/ScopeSplitPicker.qml`
- Context menu: `qml/scopeui/ScopeCommandMenu.qml`

## Matrix Behavior (Excel-like)

- Grid cố định 5x5.
- Ô từ (1,1) tới (5,5).
- Vùng từ góc trái trên đến ô hover sẽ được highlight.
- Click để chốt layout split.

## Scope Distribution After Split

Mặc định dữ liệu scope gốc được chia sang scope mới theo channel:

- Lấy danh sách channel từ `scopeController.channelIds`
- Chia round-robin theo chỉ số cell:
  - channel index `i` thuộc scope cell `(i % scopeCount)`

Ví dụ:
- 4 channels, split 2x2 => mỗi scope thường nhận 1 channel
- 4 channels, split 3x3 => một số scope không có channel (hiển thị trống)

## Active Scope Target

- Click vào một scope con để đặt scope đó làm `viewToolTarget`.
- Các tool Zoom/Fit/Pan tác động lên scope đang active.

## Legend cho từng đường

- Mỗi scope hiển thị legend ở góc trên bên phải.
- Mỗi dòng legend có:
  - màu đường
  - label `CH <channelId>`
- Màu legend dùng cùng palette với renderer `ScopeView`, nên người dùng đối chiếu line/label trực tiếp.
- Legend lấy danh sách channel đã phân bổ cho scope hiện tại (sau split vẫn đúng theo round-robin).
- Click vào tên channel trong legend sẽ toggle ẩn/hiện line ngay trên scope tương ứng.
- Khi bị ẩn, label vẫn giữ trên legend và hiển thị dạng gạch (strike-through).

## Right-click Context Menu

Menu chuột phải gồm 3 phần:

1. `History Follow` (flag/toggle)
- Bật: scope active ở auto-follow mode.
- Tắt: scope active ở manual history mode.

2. `Inputs` tab
- Click `Inputs` để mở danh sách toàn bộ input hiện có (`controller.channelIds`).
- Mục đầu tiên là `All` để chọn nhanh toàn bộ input cho scope active.
- Mỗi input (`CH <id>`) là một toggle để thêm/bớt input trên scope active.
- Legend trong scope cập nhật theo danh sách input đang bật.

3. `Tools` tab
- Click `Tools` để chuyển sang danh sách command tools.
- Click command tool để chạy và hiện popup kết quả.

Command tool hiện tại:
- `Mean/Max/Min`
  - Tính mean/max/min theo từng channel trong range hiện tại.
  - Range và channel lấy theo scope đang active.
