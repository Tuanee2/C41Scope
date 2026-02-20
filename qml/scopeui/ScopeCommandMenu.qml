import QtQuick

Item {
    id: root

    required property var controller
    property bool menuVisible: false
    property real menuX: 0
    property real menuY: 0
    property bool showingTools: false

    readonly property var activeScope: root.controller ? root.controller.viewToolTarget : null
    readonly property bool historyFollow: root.activeScope ? root.activeScope.liveMode : true

    signal dismissed()

    visible: menuVisible

    function closeMenu() {
        root.showingTools = false
        root.dismissed()
    }

    onMenuVisibleChanged: {
        if (menuVisible) {
            showingTools = false
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.menuVisible
        onClicked: {
            root.closeMenu()
        }
    }

    Rectangle {
        id: contextMenu
        visible: root.menuVisible
        x: Math.min(Math.max(8, root.menuX), root.width - width - 8)
        y: Math.min(Math.max(48, root.menuY), root.height - height - 8)
        width: 240
        radius: 8
        color: "#121b29"
        border.width: 1
        border.color: "#3c506b"

        Column {
            id: menuColumn
            x: 6
            y: 6
            width: parent.width - 12
            spacing: 4

            Rectangle {
                visible: !root.showingTools
                width: menuColumn.width
                height: 30
                radius: 6
                color: historyMouse.containsMouse ? "#2d3c51" : "#1b2736"
                border.width: 1
                border.color: "#34485f"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    color: "#d7e3f4"
                    font.pixelSize: 12
                    text: "History Follow"
                }

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    width: 34
                    height: 18
                    radius: 9
                    color: root.historyFollow ? "#6ea4ff" : "#2a384a"

                    Rectangle {
                        width: 14
                        height: 14
                        radius: 7
                        anchors.verticalCenter: parent.verticalCenter
                        x: root.historyFollow ? 18 : 2
                        color: "#f3f8ff"
                    }
                }

                MouseArea {
                    id: historyMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        if (root.activeScope) {
                            root.activeScope.liveMode = !root.historyFollow
                        }
                    }
                }
            }

            Rectangle {
                visible: !root.showingTools
                width: menuColumn.width
                height: 30
                radius: 6
                color: toolsMouse.containsMouse ? "#2d3c51" : "#1b2736"
                border.width: 1
                border.color: "#34485f"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    color: "#d7e3f4"
                    font.pixelSize: 12
                    text: "Tools"
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    color: "#a9c2e3"
                    font.pixelSize: 12
                    text: ">"
                }

                MouseArea {
                    id: toolsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.showingTools = true
                    }
                }
            }

            Rectangle {
                visible: root.showingTools
                width: menuColumn.width
                height: 30
                radius: 6
                color: backMouse.containsMouse ? "#2d3c51" : "#1b2736"
                border.width: 1
                border.color: "#34485f"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    color: "#d7e3f4"
                    font.pixelSize: 12
                    text: "< Back"
                }

                MouseArea {
                    id: backMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.showingTools = false
                    }
                }
            }

            Repeater {
                model: root.showingTools && root.controller ? root.controller.commandToolItems : []

                delegate: Rectangle {
                    required property var modelData
                    width: menuColumn.width
                    height: 28
                    radius: 6
                    color: commandMouse.containsMouse ? "#2d3c51" : "#1b2736"
                    border.width: 1
                    border.color: "#34485f"

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        color: "#d7e3f4"
                        font.pixelSize: 12
                        text: modelData.label
                    }

                    MouseArea {
                        id: commandMouse
                        anchors.fill: parent
                        hoverEnabled: true
                    onClicked: {
                        if (root.controller) {
                            root.controller.triggerCommandTool(modelData.toolId)
                        }
                        root.closeMenu()
                    }
                }
            }
            }

            Rectangle {
                visible: root.showingTools && root.controller ? (root.controller.commandToolItems.length === 0) : false
                width: menuColumn.width
                height: 28
                radius: 6
                color: "#1b2736"
                border.width: 1
                border.color: "#34485f"

                Text {
                    anchors.centerIn: parent
                    color: "#9fb4d0"
                    font.pixelSize: 12
                    text: "No command tools"
                }
            }
        }

        height: menuColumn.implicitHeight + 12
    }
}
