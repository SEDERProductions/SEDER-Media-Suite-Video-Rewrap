import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

Slider {
    id: control

    implicitWidth: 120
    implicitHeight: 20
    hoverEnabled: true

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: 3
        radius: 1.5
        color: Theme.inset
        border.color: Theme.border
        border.width: 1

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 1.5
            color: Theme.accent
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 12
        implicitHeight: 12
        radius: 6
        color: control.pressed ? Theme.accentActive
            : control.hovered ? Theme.accentHover : Theme.controlBgHover
        border.color: control.pressed || control.hovered ? Theme.accent : Theme.borderLight
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }
}
