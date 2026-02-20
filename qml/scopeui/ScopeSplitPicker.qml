import QtQuick

Item {
    id: root

    property bool pickerVisible: false
    property real panelX: 0
    property real panelY: 0
    property int hoverRows: 1
    property int hoverCols: 1

    signal splitChosen(int rows, int cols)
    signal dismissed()

    visible: pickerVisible

    MouseArea {
        anchors.fill: parent
        enabled: root.pickerVisible
        onClicked: {
            root.dismissed()
        }
    }

    Rectangle {
        id: splitPicker
        visible: root.pickerVisible
        x: root.panelX
        y: root.panelY
        radius: 10
        color: "#111a27"
        border.width: 1
        border.color: "#364d6a"

        Column {
            x: 10
            y: 10
            spacing: 8

            Text {
                color: "#d7e3f4"
                font.pixelSize: 12
                text: "Split " + root.hoverRows + "x" + root.hoverCols
            }

            Grid {
                id: splitGrid
                rows: 5
                columns: 5
                spacing: 4

                Repeater {
                    model: 25

                    delegate: Rectangle {
                        required property int index
                        readonly property int rowIndex: Math.floor(index / 5) + 1
                        readonly property int colIndex: (index % 5) + 1
                        readonly property bool selected: rowIndex <= root.hoverRows && colIndex <= root.hoverCols

                        width: 22
                        height: 22
                        radius: 4
                        color: selected ? "#6ea4ff" : "#1b2736"
                        border.width: 1
                        border.color: selected ? "#9cc0ff" : "#34485f"

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: {
                                root.hoverRows = parent.rowIndex
                                root.hoverCols = parent.colIndex
                            }
                            onPositionChanged: {
                                root.hoverRows = parent.rowIndex
                                root.hoverCols = parent.colIndex
                            }
                            onClicked: {
                                root.splitChosen(parent.rowIndex, parent.colIndex)
                            }
                        }
                    }
                }
            }
        }

        width: splitGrid.implicitWidth + 20
        height: splitGrid.implicitHeight + 44
    }
}
