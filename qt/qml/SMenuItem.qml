import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

MenuItem {
    id: control

    property string shortcutText: ""

    implicitHeight: 26
    implicitWidth: itemRow.implicitWidth + Theme.s4 * 2
    hoverEnabled: true

    font.pixelSize: Theme.fontSizeBody
    font.family: Theme.uiFont

    Accessible.role: Accessible.MenuItem
    Accessible.name: text

    contentItem: Item {
        implicitWidth: itemRow.implicitWidth
        implicitHeight: itemRow.implicitHeight
        Row {
            id: itemRow
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.s3

            Item {
                width: 12
                height: 12
                anchors.verticalCenter: parent.verticalCenter
                visible: control.checkable
                SIcon {
                    anchors.centerIn: parent
                    width: 11
                    height: 11
                    name: "check"
                    color: control.highlighted ? Theme.onAccent : Theme.accentHover
                    visible: control.checked
                }
            }

            Text {
                text: control.text
                font: control.font
                color: !control.enabled ? Theme.textDisabled
                    : control.highlighted ? Theme.onAccent : Theme.text
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.s2

            Text {
                visible: control.shortcutText !== ""
                text: control.shortcutText
                font.family: Theme.monoFont
                font.pixelSize: Theme.fontSizeTiny
                color: control.highlighted ? Qt.rgba(1, 1, 1, 0.75) : Theme.faint
                anchors.verticalCenter: parent.verticalCenter
            }

            SIcon {
                visible: control.subMenu !== null
                name: "chevronRight"
                width: 11
                height: 11
                color: control.highlighted ? Theme.onAccent : Theme.muted
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    background: Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        radius: Theme.radius
        color: control.highlighted ? Theme.accent : "transparent"
    }
}
