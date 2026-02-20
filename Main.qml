import QtQuick
import QtQuick.Window
import C41Scope 1.0

Window {
    width: 1200
    height: 720
    visible: true
    title: "C41Scope Skeleton"

    ScopeWorkbench {
        anchors.fill: parent
        controller: scopeController
    }
}
