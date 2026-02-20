import QtQuick

Item {
    id: root

    property bool dialogVisible: false
    property string title: ""
    property string body: ""
    signal dismissed()

    visible: dialogVisible

    Rectangle {
        anchors.fill: parent
        color: "#00000066"
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.dialogVisible
        onClicked: {
            root.dismissed()
        }
    }

    Rectangle {
        width: Math.min(parent.width * 0.72, 760)
        height: Math.min(parent.height * 0.72, 500)
        anchors.centerIn: parent
        radius: 10
        color: "#121b29"
        border.width: 1
        border.color: "#3c506b"

        Rectangle {
            id: resultHeader
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 42
            color: "#182433"

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 14
                color: "#d7e3f4"
                font.pixelSize: 14
                text: root.title
            }

            Rectangle {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                width: 24
                height: 24
                radius: 6
                color: closeResultMouse.containsMouse ? "#2d3c51" : "#1f2c3c"

                Text {
                    anchors.centerIn: parent
                    color: "#d7e3f4"
                    font.pixelSize: 14
                    text: "x"
                }

                MouseArea {
                    id: closeResultMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.dismissed()
                    }
                }
            }
        }

        Flickable {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: resultHeader.bottom
            anchors.bottom: parent.bottom
            anchors.margins: 12
            clip: true
            contentWidth: width
            contentHeight: resultText.paintedHeight + 12

            Text {
                id: resultText
                width: parent.width
                wrapMode: Text.Wrap
                color: "#c5d5ea"
                font.family: "Menlo"
                font.pixelSize: 12
                text: root.body
            }
        }
    }
}
