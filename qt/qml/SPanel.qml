import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

// Premiere-style panel: charcoal body with a slim header strip.
// Place content in `content`; add header controls via `headerItems`.
Rectangle {
    id: panel

    property string title: ""
    default property alias content: contentHost.data
    property alias headerItems: headerRow.data
    property bool flat: false

    color: Theme.panel
    border.color: Theme.border
    border.width: flat ? 0 : 1
    radius: 0

    Rectangle {
        id: header
        visible: panel.title !== ""
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: panel.flat ? 0 : 1
        height: visible ? 24 : 0
        color: Theme.headerStrip

        Text {
            anchors.left: parent.left
            anchors.leftMargin: Theme.s3
            anchors.verticalCenter: parent.verticalCenter
            text: panel.title
            color: Theme.muted
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeSmall
            font.bold: true
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.8
            elide: Text.ElideRight
        }

        Row {
            id: headerRow
            anchors.right: parent.right
            anchors.rightMargin: Theme.s2
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.s1
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.border
        }
    }

    Item {
        id: contentHost
        anchors.top: header.visible ? header.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: panel.flat ? 0 : 1
    }
}
