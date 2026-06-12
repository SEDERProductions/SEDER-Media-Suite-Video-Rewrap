import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

// Tooltip with an optional keyboard-shortcut hint line, Premiere-style.
ToolTip {
    id: control

    property string shortcutHint: ""

    delay: 600
    timeout: 7000

    contentItem: Column {
        spacing: 2
        Text {
            text: control.text
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.text
            wrapMode: Text.WordWrap
            width: Math.min(implicitWidth, 280)
        }
        Text {
            visible: control.shortcutHint !== ""
            text: control.shortcutHint
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeTiny
            color: Theme.muted
        }
    }

    background: Rectangle {
        color: Theme.dark ? "#0d0d0d" : "#ffffff"
        border.color: Theme.borderLight
        border.width: 1
        radius: Theme.radius
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durMid }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durFast }
    }
}
