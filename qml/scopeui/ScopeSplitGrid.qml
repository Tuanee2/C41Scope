import QtQuick
import C41Scope 1.0

Item {
    id: root

    required property var controller
    property int splitRows: 1
    property int splitCols: 1
    readonly property int scopeCount: splitRows * splitCols

    signal commandMenuRequested(real x, real y)

    function channelsForCell(cellIndex) {
        const ids = root.controller ? root.controller.channelIds : []
        if (!ids || ids.length === 0) {
            return []
        }

        if (scopeCount <= 1) {
            return ids
        }

        const assigned = []
        for (let i = 0; i < ids.length; ++i) {
            if ((i % scopeCount) === cellIndex) {
                assigned.push(ids[i])
            }
        }
        return assigned
    }

    Grid {
        id: scopeGrid
        anchors.fill: parent
        columns: root.splitCols
        spacing: 8

        Repeater {
            model: root.scopeCount

            delegate: Rectangle {
                required property int index
                readonly property bool isActive: root.controller && root.controller.viewToolTarget === scopeView

                width: Math.max(120, (scopeGrid.width - (scopeGrid.spacing * (root.splitCols - 1))) / root.splitCols)
                height: Math.max(90, (scopeGrid.height - (scopeGrid.spacing * (root.splitRows - 1))) / root.splitRows)
                radius: 8
                color: "#101927"
                border.width: isActive ? 2 : 1
                border.color: isActive ? "#6ea4ff" : "#2c3f56"

                ScopeView {
                    id: scopeView
                    anchors.fill: parent
                    anchors.margins: 2
                    controller: root.controller
                    channelIds: root.channelsForCell(index)
                    timeWindow: 5.0
                    liveMode: true
                    paused: false
                    showGrid: true

                    Component.onCompleted: {
                        if (index === 0 && root.controller && root.controller.viewToolTarget === null) {
                            root.controller.viewToolTarget = scopeView
                        }
                    }
                }

                MouseArea {
                    id: scopeMouse
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    property real lastDragX: 0
                    onPressed: function(mouse) {
                        if (!root.controller) {
                            return
                        }

                        root.controller.viewToolTarget = scopeView
                        scopeMouse.lastDragX = mouse.x
                        if (mouse.button === Qt.RightButton) {
                            const p = scopeView.mapToItem(root, mouse.x, mouse.y)
                            root.commandMenuRequested(p.x, p.y)
                            mouse.accepted = true
                        }
                    }
                    onPositionChanged: function(mouse) {
                        if (!root.controller || (mouse.buttons & Qt.LeftButton) === 0) {
                            return
                        }

                        if (root.controller.viewToolTarget !== scopeView || scopeView.liveMode) {
                            scopeMouse.lastDragX = mouse.x
                            return
                        }

                        const dx = mouse.x - scopeMouse.lastDragX
                        if (Math.abs(dx) >= 0.2) {
                            scopeView.panByPixels(dx, scopeView.width)
                            scopeMouse.lastDragX = mouse.x
                        }
                    }
                }
            }
        }
    }
}
