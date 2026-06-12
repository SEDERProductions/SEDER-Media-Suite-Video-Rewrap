import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

CheckBox {
    id: control

    implicitHeight: 24
    spacing: Theme.s3
    hoverEnabled: true

    font.pixelSize: Theme.fontSizeBody
    font.family: Theme.uiFont

    Accessible.role: Accessible.CheckBox
    Accessible.name: text

    indicator: Rectangle {
        implicitWidth: 15
        implicitHeight: 15
        x: control.leftPadding
        y: parent.height / 2 - height / 2
        radius: Theme.radius
        color: control.checked ? Theme.accent
            : control.hovered ? Theme.controlBgHover : Theme.controlBg
        border.color: control.visualFocus ? Theme.accentHover
            : control.checked ? Theme.accent : Theme.border
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }

        SIcon {
            anchors.centerIn: parent
            width: 11
            height: 11
            name: "check"
            color: Theme.onAccent
            visible: control.checked
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.enabled ? Theme.text : Theme.textDisabled
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
        elide: Text.ElideRight
    }
}
