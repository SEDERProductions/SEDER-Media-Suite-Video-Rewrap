import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

ScrollBar {
    id: control

    property bool alwaysVisible: false

    policy: alwaysVisible ? ScrollBar.AlwaysOn : ScrollBar.AsNeeded
    minimumSize: 0.08
    hoverEnabled: true

    implicitWidth: orientation === Qt.Vertical ? 10 : 0
    implicitHeight: orientation === Qt.Horizontal ? 10 : 0

    contentItem: Rectangle {
        implicitWidth: control.orientation === Qt.Vertical ? 6 : 100
        implicitHeight: control.orientation === Qt.Horizontal ? 6 : 100
        radius: 3
        color: control.pressed ? Theme.faint
            : control.hovered ? Theme.borderLight : Theme.border
        opacity: control.policy === ScrollBar.AlwaysOn || control.active ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: Theme.durMid } }
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }

    background: null
}
