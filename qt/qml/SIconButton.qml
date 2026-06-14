import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

// Square icon-only button used in transport bars and panel headers.
Button {
    id: control

    property string iconName: ""
    property string tooltip: ""
    property string shortcutHint: ""
    property bool accented: false
    property int iconSize: 14

    implicitWidth: 26
    implicitHeight: 26
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    Accessible.role: Accessible.Button
    Accessible.name: tooltip !== "" ? tooltip : iconName

    contentItem: Item {
        SIcon {
            anchors.centerIn: parent
            name: control.iconName
            width: control.iconSize
            height: control.iconSize
            color: !control.enabled ? Theme.textDisabled
                : control.accented ? Theme.accent
                : control.hovered ? Theme.text : Theme.muted
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
        }
    }

    background: Rectangle {
        radius: Theme.radius
        color: !control.enabled ? "transparent"
            : control.pressed ? Theme.controlBgPressed
            : control.hovered ? Theme.controlBgHover
            : "transparent"
        border.color: control.visualFocus ? Theme.accent : "transparent"
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }

    SToolTip {
        visible: control.hovered && control.tooltip !== ""
        text: control.tooltip
        shortcutHint: control.shortcutHint
    }
}
