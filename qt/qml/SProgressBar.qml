import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

ProgressBar {
    id: control

    implicitWidth: 160
    implicitHeight: 5

    background: Rectangle {
        radius: 2.5
        color: Theme.inset
        border.color: Theme.border
        border.width: 1
    }

    contentItem: Item {
        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 2.5
            color: Theme.accent
            visible: !control.indeterminate
            Behavior on width { NumberAnimation { duration: Theme.durFast } }
        }
        Rectangle {
            // Indeterminate sweep
            property real cycle: 0
            width: parent.width * 0.3
            height: parent.height
            x: cycle * parent.width * 0.7
            radius: 2.5
            color: Theme.accent
            visible: control.indeterminate
            NumberAnimation on cycle {
                running: control.indeterminate && control.visible
                from: 0; to: 1
                duration: 1100
                loops: Animation.Infinite
                easing.type: Easing.InOutQuad
            }
        }
    }
}
