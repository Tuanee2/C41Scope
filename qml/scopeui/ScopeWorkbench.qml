import QtQuick
import C41Scope 1.0

Item {
    id: root

    required property var controller

    property int splitRows: 1
    property int splitCols: 1
    property bool splitPickerVisible: false
    property int splitHoverRows: 1
    property int splitHoverCols: 1

    property bool commandMenuVisible: false
    property real commandMenuX: 20
    property real commandMenuY: 80

    property bool commandResultVisible: false
    property string commandResultTitle: ""
    property string commandResultBody: ""
    readonly property bool activeHistoryFollow: root.controller && root.controller.viewToolTarget
                                                ? root.controller.viewToolTarget.liveMode
                                                : true

    function applySplit(rows, cols) {
        splitRows = Math.max(1, Math.min(5, rows))
        splitCols = Math.max(1, Math.min(5, cols))
        splitPickerVisible = false
    }

    function openCommandMenuAt(localX, localY) {
        const commandCount = root.controller ? root.controller.commandToolItems.length : 0
        const rowCount = Math.max(2, commandCount + 1)
        const menuWidth = 240
        const menuHeight = 12 + (rowCount * 32)

        const absX = scopeContainer.x + localX
        const absY = scopeContainer.y + localY
        commandMenuX = Math.min(Math.max(8, absX), root.width - menuWidth - 8)
        commandMenuY = Math.min(Math.max(48, absY), root.height - menuHeight - 8)
        commandMenuVisible = true
        splitPickerVisible = false
    }

    Connections {
        target: root.controller
        function onSplitToolRequested() {
            root.splitHoverRows = root.splitRows
            root.splitHoverCols = root.splitCols
            root.splitPickerVisible = true
            root.commandMenuVisible = false
        }

        function onCommandToolResultReady(title, body) {
            root.commandResultTitle = title
            root.commandResultBody = body
            root.commandResultVisible = true
            root.commandMenuVisible = false
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#0b1017"
    }

    Item {
        id: scopeContainer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 20
        anchors.topMargin: 56

        ScopeSplitGrid {
            id: splitGrid
            anchors.fill: parent
            controller: root.controller
            splitRows: root.splitRows
            splitCols: root.splitCols
            onCommandMenuRequested: function(x, y) {
                root.openCommandMenuAt(x, y)
            }
        }
    }

    ScopeToolbar {
        id: viewToolMenu
        anchors.left: scopeContainer.left
        anchors.top: scopeContainer.top
        anchors.leftMargin: 8
        anchors.topMargin: 8
        z: 30
        controller: root.controller
        splitRows: root.splitRows
        splitCols: root.splitCols
        activeHistoryFollow: root.activeHistoryFollow
        activeWindowSeconds: root.controller && root.controller.viewToolTarget
                             ? root.controller.viewToolTarget.timeWindow
                             : 0.0
    }

    ScopeSplitPicker {
        anchors.fill: parent
        z: 35
        pickerVisible: root.splitPickerVisible
        panelX: viewToolMenu.x
        panelY: viewToolMenu.y + viewToolMenu.height + 8
        hoverRows: root.splitHoverRows
        hoverCols: root.splitHoverCols
        onSplitChosen: function(rows, cols) {
            root.applySplit(rows, cols)
        }
        onDismissed: {
            root.splitPickerVisible = false
        }
    }

    ScopeCommandMenu {
        anchors.fill: parent
        z: 36
        controller: root.controller
        menuVisible: root.commandMenuVisible
        menuX: root.commandMenuX
        menuY: root.commandMenuY
        onDismissed: {
            root.commandMenuVisible = false
        }
    }

    ScopeCommandResultDialog {
        anchors.fill: parent
        z: 60
        dialogVisible: root.commandResultVisible
        title: root.commandResultTitle
        body: root.commandResultBody
        onDismissed: {
            root.commandResultVisible = false
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 40
        color: "#121923"
        opacity: 0.9
        z: 50

        Text {
            anchors.centerIn: parent
            color: "#d7e3f4"
            font.pixelSize: 14
            text: "Realtime QSG Skeleton - 400Hz fake ingest"
        }
    }
}
