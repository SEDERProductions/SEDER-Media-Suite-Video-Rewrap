import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

TextField {
    id: control

    property bool mono: false

    implicitHeight: 26
    leftPadding: Theme.s3
    rightPadding: Theme.s3
    hoverEnabled: true

    color: enabled ? Theme.text : Theme.textDisabled
    placeholderTextColor: Theme.faint
    selectedTextColor: Theme.textOnAccent
    selectionColor: Theme.accent
    font.pixelSize: Theme.fontSizeBody
    font.family: mono ? Theme.monoFont : Theme.uiFont
    verticalAlignment: TextInput.AlignVCenter

    background: Rectangle {
        radius: Theme.radius
        color: Theme.inset
        border.color: control.activeFocus ? Theme.accent
            : control.hovered ? Theme.borderLight : Theme.border
        border.width: 1
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }
}
