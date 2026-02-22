import QtQuick

Item {
    id: root

    required property var controller
    property bool menuVisible: false
    property real menuX: 0
    property real menuY: 0
    property string page: "main"

    readonly property var activeScope: root.controller ? root.controller.viewToolTarget : null
    readonly property bool historyFollow: root.activeScope ? root.activeScope.liveMode : true
    readonly property var availableInputs: root.controller ? root.controller.channelIds : []

    signal dismissed()

    visible: menuVisible

    function normalizeInputList(values) {
        const normalized = []
        if (!values) {
            return normalized
        }

        for (let i = 0; i < values.length; ++i) {
            const value = Number(values[i])
            if (!Number.isFinite(value)) {
                continue
            }

            const channelId = Math.trunc(value)
            if (normalized.indexOf(channelId) < 0) {
                normalized.push(channelId)
            }
        }

        normalized.sort(function(a, b) { return a - b })
        return normalized
    }

    function isInputSelected(channelId) {
        if (!root.activeScope) {
            return false
        }

        const selectedInputs = root.normalizeInputList(root.activeScope.channelIds)
        return selectedInputs.indexOf(channelId) >= 0
    }

    function areAllInputsSelected() {
        if (!root.activeScope) {
            return false
        }

        const allInputs = root.normalizeInputList(root.availableInputs)
        if (allInputs.length === 0) {
            return false
        }

        for (let i = 0; i < allInputs.length; ++i) {
            if (!root.isInputSelected(allInputs[i])) {
                return false
            }
        }

        return true
    }

    function applyInputSelection(inputs) {
        if (!root.activeScope) {
            return
        }

        root.activeScope.channelIds = root.normalizeInputList(inputs)
    }

    function toggleInput(channelId) {
        if (!root.activeScope) {
            return
        }

        const selectedInputs = root.normalizeInputList(root.activeScope.channelIds)
        const index = selectedInputs.indexOf(channelId)
        if (index >= 0) {
            selectedInputs.splice(index, 1)
        } else {
            selectedInputs.push(channelId)
        }

        root.applyInputSelection(selectedInputs)
    }

    function selectAllInputs() {
        root.applyInputSelection(root.availableInputs)
    }

    function closeMenu() {
        root.page = "main"
        root.dismissed()
    }

    onMenuVisibleChanged: {
        if (menuVisible) {
            page = "main"
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
                visible: root.page === "main"
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
                visible: root.page === "main"
                width: menuColumn.width
                height: 30
                radius: 6
                color: inputsMouse.containsMouse ? "#2d3c51" : "#1b2736"
                border.width: 1
                border.color: "#34485f"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    color: "#d7e3f4"
                    font.pixelSize: 12
                    text: "Inputs"
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
                    id: inputsMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.page = "inputs"
                    }
                }
            }

            Rectangle {
                visible: root.page === "main"
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
                        root.page = "tools"
                    }
                }
            }

            Rectangle {
                visible: root.page !== "main"
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
                        root.page = "main"
                    }
                }
            }

            Repeater {
                model: root.page === "inputs" && root.activeScope ? [0] : []

                delegate: Rectangle {
                    width: menuColumn.width
                    height: 28
                    radius: 6
                    color: allInputsMouse.containsMouse ? "#2d3c51" : "#1b2736"
                    border.width: 1
                    border.color: "#34485f"

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        color: "#d7e3f4"
                        font.pixelSize: 12
                        text: "All"
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        color: root.areAllInputsSelected() ? "#9cc0ff" : "#6a819f"
                        font.pixelSize: 12
                        text: root.areAllInputsSelected() ? "x" : "-"
                    }

                    MouseArea {
                        id: allInputsMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.selectAllInputs()
                        }
                    }
                }
            }

            Repeater {
                model: root.page === "inputs" ? root.normalizeInputList(root.availableInputs) : []

                delegate: Rectangle {
                    required property var modelData
                    readonly property int inputId: Number(modelData)
                    width: menuColumn.width
                    height: 28
                    radius: 6
                    color: inputMouse.containsMouse ? "#2d3c51" : "#1b2736"
                    border.width: 1
                    border.color: "#34485f"

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        color: "#d7e3f4"
                        font.pixelSize: 12
                        text: "CH " + parent.inputId
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        color: root.isInputSelected(parent.inputId) ? "#9cc0ff" : "#6a819f"
                        font.pixelSize: 12
                        text: root.isInputSelected(parent.inputId) ? "x" : "-"
                    }

                    MouseArea {
                        id: inputMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.toggleInput(parent.inputId)
                        }
                    }
                }
            }

            Rectangle {
                visible: root.page === "inputs" ? (root.normalizeInputList(root.availableInputs).length === 0) : false
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
                    text: "No inputs"
                }
            }

            Repeater {
                model: root.page === "tools" && root.controller ? root.controller.commandToolItems : []

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
                visible: root.page === "tools" && root.controller ? (root.controller.commandToolItems.length === 0) : false
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
