import QtQuick
import QtQuick.Layouts
import Seder.VideoRewrap

Rectangle {
    id: banner

    visible: app.updateChecker.updateAvailable
    implicitHeight: visible ? 34 : 0
    color: Qt.rgba(Theme.warn.r, Theme.warn.g, Theme.warn.b, Theme.dark ? 0.16 : 0.2)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.s4
        anchors.rightMargin: Theme.s4
        spacing: Theme.s3

        SIcon {
            name: "download"
            width: 13
            height: 13
            color: Theme.warn
        }

        Text {
            text: qsTr("Update available: %1").arg(app.updateChecker.latestVersion)
            color: Theme.text
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeBody
            font.bold: true
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        SButton {
            text: qsTr("Download")
            iconName: "externalLink"
            onClicked: app.updateChecker.openUpdateUrl()
        }

        SIconButton {
            iconName: "close"
            tooltip: qsTr("Dismiss")
            onClicked: app.updateChecker.dismiss()
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.border
    }
}
