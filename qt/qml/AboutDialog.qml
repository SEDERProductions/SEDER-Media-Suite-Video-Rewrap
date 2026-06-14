import QtQuick
import QtQuick.Layouts
import Seder.VideoRewrap

SDialog {
    id: dialog

    titleText: qsTr("About SEDER Video Rewrap")
    width: 440

    RowLayout {
        spacing: Theme.s4
        Layout.topMargin: Theme.s3

        Image {
            source: "qrc:/branding/icon-64.png"
            sourceSize: Qt.size(48, 48)
            Layout.alignment: Qt.AlignTop
        }

        ColumnLayout {
            spacing: Theme.s1
            Layout.fillWidth: true

            Text {
                text: qsTr("SEDER Media Suite Video Rewrap")
                font.family: Theme.uiFont
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                color: Theme.text
            }
            Text {
                text: qsTr("Version %1").arg(app.appVersion)
                font.family: Theme.monoFont
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.muted
            }
        }
    }

    Text {
        text: qsTr("Local-first Qt/Rust desktop tool for keyframe-aligned FFmpeg stream-copy exports.")
        color: Theme.muted
        font.family: Theme.uiFont
        font.pixelSize: Theme.fontSizeBody
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.maximumWidth: 400
    }

    Text {
        text: qsTr("Includes FFmpeg as an external dependency (LGPL).\nLicensed under GPL-3.0-only.")
        color: Theme.faint
        font.family: Theme.uiFont
        font.pixelSize: Theme.fontSizeSmall
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

    SCheckBox {
        text: qsTr("Check for updates on launch")
        checked: app.updateChecker.checkOnLaunch
        onToggled: app.updateChecker.checkOnLaunch = checked
    }

    RowLayout {
        spacing: Theme.s3
        Layout.bottomMargin: Theme.s3

        SButton {
            text: qsTr("Check Now")
            iconName: "refresh"
            onClicked: app.updateChecker.checkNow()
        }
        Text {
            text: app.updateChecker.lastMessage
            color: Theme.faint
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeSmall
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    footerButtons: SButton {
        text: qsTr("Close")
        onClicked: dialog.close()
    }
}
