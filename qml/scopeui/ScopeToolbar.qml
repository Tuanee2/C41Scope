import QtQuick

Rectangle {
    id: root

    required property var controller
    property int splitRows: 1
    property int splitCols: 1
    property double activeWindowSeconds: 0.0
    property bool activeHistoryFollow: true

    radius: 10
    color: "#121923"
    opacity: 0.95

    Row {
        id: toolRow
        x: 8
        y: 8
        spacing: 8

        Repeater {
            model: root.controller ? root.controller.viewToolItems : []

            delegate: Rectangle {
                required property var modelData
                width: Math.max(58, toolLabel.implicitWidth + 20)
                height: 30
                radius: 6
                color: mouseArea.containsMouse ? "#2d3c51" : "#1b2736"
                border.width: 1
                border.color: "#3c506b"

                Text {
                    id: toolLabel
                    anchors.centerIn: parent
                    color: "#d7e3f4"
                    font.pixelSize: 12
                    text: modelData.label
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (root.controller) {
                            root.controller.triggerViewTool(modelData.toolId)
                        }
                    }
                }
            }
        }

        Rectangle {
            width: 150
            height: 30
            radius: 6
            color: "#182433"
            border.width: 1
            border.color: "#3c506b"

            Text {
                anchors.centerIn: parent
                color: "#c5d5ea"
                font.pixelSize: 12
                text: root.splitRows + "x" + root.splitCols
                      + " - "
                      + (root.activeHistoryFollow ? "Follow" : "Manual")
                      + " - Window "
                      + root.activeWindowSeconds.toFixed(2)
                      + "s"
            }
        }
    }

    width: toolRow.implicitWidth + 16
    height: toolRow.implicitHeight + 16
}
