import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Seder.VideoRewrap

SDialog {
    id: dialog

    titleText: qsTr("Error Details")

    Text {
        Layout.topMargin: Theme.s3
        text: qsTr("The most recent operation did not complete. Details below — share these with support if you need help.")
        color: Theme.muted
        font.family: Theme.uiFont
        font.pixelSize: Theme.fontSizeBody
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 220
        Layout.bottomMargin: Theme.s3
        color: Theme.inset
        border.color: Theme.border
        border.width: 1
        radius: Theme.radius

        ScrollView {
            anchors.fill: parent
            anchors.margins: 1

            TextArea {
                readOnly: true
                wrapMode: TextEdit.Wrap
                font.family: Theme.monoFont
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.text
                selectionColor: Theme.accent
                selectedTextColor: Theme.textOnAccent
                text: app.lastErrorLog
                background: null
            }
        }
    }

    footerButtons: [
        SButton {
            text: qsTr("Close")
            onClicked: dialog.close()
        },
        SButton {
            text: qsTr("Copy Log")
            iconName: "duplicate"
            onClicked: app.copyLastErrorLogToClipboard()
        }
    ]
}
