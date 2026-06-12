import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

Button {
    id: control

    property bool primary: false
    property bool danger: false
    property string iconName: ""
    property string tooltip: ""
    property string shortcutHint: ""

    readonly property color labelColor: !enabled
        ? Theme.textDisabled
        : (primary || danger) ? Theme.onAccent : Theme.text

    implicitHeight: 28
    implicitWidth: Math.max(contentRow.implicitWidth + leftPadding + rightPadding, 64)
    leftPadding: Theme.s4
    rightPadding: Theme.s4
    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    font.pixelSize: Theme.fontSizeBody
    font.family: Theme.uiFont

    Accessible.role: Accessible.Button
    Accessible.name: text

    contentItem: Item {
        implicitWidth: contentRow.implicitWidth
        implicitHeight: contentRow.implicitHeight
        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: Theme.s2 + 2
            SIcon {
                visible: control.iconName !== ""
                name: control.iconName
                width: 14
                height: 14
                anchors.verticalCenter: parent.verticalCenter
                color: control.labelColor
            }
            Text {
                visible: control.text !== ""
                text: control.text
                font: control.font
                color: control.labelColor
                anchors.verticalCenter: parent.verticalCenter
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
            }
        }
    }

    background: Rectangle {
        radius: Theme.radius
        color: {
            if (!control.enabled) return Theme.controlBgDisabled
            var base = control.primary ? Theme.accent
                     : control.danger ? Theme.bad
                     : Theme.controlBg
            if (control.pressed)
                return control.primary ? Theme.accentActive
                     : control.danger ? Qt.darker(Theme.bad, 1.2)
                     : Theme.controlBgPressed
            if (control.hovered)
                return control.primary ? Theme.accentHover
                     : control.danger ? Qt.lighter(Theme.bad, 1.1)
                     : Theme.controlBgHover
            return base
        }
        border.color: control.visualFocus ? Theme.accent
            : (control.primary || control.danger) ? "transparent" : Theme.border
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
    }

    SToolTip {
        visible: control.hovered && control.tooltip !== ""
        text: control.tooltip
        shortcutHint: control.shortcutHint
    }
}
