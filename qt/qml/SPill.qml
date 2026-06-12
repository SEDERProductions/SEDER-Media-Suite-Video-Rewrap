import QtQuick
import Seder.VideoRewrap

// Compact status badge: READY / MISSING / WORKING etc.
Rectangle {
    id: pill

    property string text: ""
    property color tone: Theme.faint
    property string iconName: ""

    implicitHeight: 18
    implicitWidth: row.implicitWidth + Theme.s4
    radius: 9
    color: Qt.rgba(tone.r, tone.g, tone.b, Theme.dark ? 0.18 : 0.14)
    border.color: Qt.rgba(tone.r, tone.g, tone.b, 0.65)
    border.width: 1

    Accessible.role: Accessible.StaticText
    Accessible.name: text

    Row {
        id: row
        anchors.centerIn: parent
        spacing: Theme.s2
        SIcon {
            visible: pill.iconName !== ""
            name: pill.iconName
            width: 9
            height: 9
            color: pill.tone
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: pill.text
            color: pill.tone
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeTiny
            font.bold: true
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.5
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
