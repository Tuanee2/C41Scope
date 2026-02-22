import QtQuick

Rectangle {
    id: root

    property var channelIds: []
    property var hiddenChannelIds: []
    property int maxVisibleItems: 8
    readonly property int totalCount: channelIds ? channelIds.length : 0
    readonly property int visibleCount: Math.min(maxVisibleItems, totalCount)
    signal channelLabelClicked(int channelId)

    function colorForChannel(channelId) {
        const palette = ["#0f9d58", "#db4437", "#4285f4", "#f4b400", "#00acc1", "#8e24aa"]
        const normalized = Math.abs(channelId)
        return palette[normalized % palette.length]
    }

    function isChannelHidden(channelId) {
        if (!hiddenChannelIds) {
            return false
        }

        for (let i = 0; i < hiddenChannelIds.length; ++i) {
            if (Number(hiddenChannelIds[i]) === channelId) {
                return true
            }
        }
        return false
    }

    visible: visibleCount > 0
    radius: 6
    color: "#0d1522cc"
    border.width: 1
    border.color: "#2d425a"

    Column {
        id: legendColumn
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 6
        anchors.topMargin: 6
        spacing: 4

        Repeater {
            model: root.visibleCount

            delegate: Rectangle {
                id: channelEntry
                required property int index
                readonly property int channelId: Number(root.channelIds[index])
                width: marker.width + labelText.implicitWidth + 18
                height: 18
                radius: 4
                color: rowMouse.containsMouse ? "#203247" : "transparent"

                Row {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 4
                    spacing: 6

                    Rectangle {
                        id: marker
                        width: 12
                        height: 3
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 1
                        color: root.colorForChannel(channelEntry.channelId)
                    }

                    Text {
                        id: labelText
                        anchors.verticalCenter: parent.verticalCenter
                        color: root.isChannelHidden(channelEntry.channelId) ? "#7f94ad" : "#d7e3f4"
                        font.pixelSize: 11
                        font.strikeout: root.isChannelHidden(channelEntry.channelId)
                        text: "CH " + channelEntry.channelId
                    }
                }

                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.channelLabelClicked(channelEntry.channelId)
                    }
                }
            }
        }

        Text {
            visible: root.totalCount > root.maxVisibleItems
            color: "#9fb4d0"
            font.pixelSize: 10
            text: "+" + (root.totalCount - root.maxVisibleItems) + " more"
        }
    }

    width: legendColumn.implicitWidth + 12
    height: legendColumn.implicitHeight + 12
}
