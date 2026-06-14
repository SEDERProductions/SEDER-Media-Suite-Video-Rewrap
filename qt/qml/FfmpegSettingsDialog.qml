import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import Seder.VideoRewrap

SDialog {
    id: dialog

    titleText: qsTr("FFmpeg Path")
    width: 480

    Text {
        Layout.topMargin: Theme.s3
        text: qsTr("If FFmpeg is not on PATH, point the app at the folder containing ffmpeg, ffprobe, and ffplay.")
        color: Theme.muted
        font.family: Theme.uiFont
        font.pixelSize: Theme.fontSizeBody
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.maximumWidth: 440
    }

    RowLayout {
        spacing: Theme.s3

        Text {
            text: qsTr("Current:")
            color: Theme.faint
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeSmall
        }
        Text {
            text: app.customFfmpegDir.length ? app.customFfmpegDir : qsTr("(using system PATH)")
            color: Theme.text
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeSmall
            elide: Text.ElideMiddle
            Layout.fillWidth: true
        }
    }

    RowLayout {
        spacing: Theme.s3

        SButton {
            text: qsTr("Choose Folder…")
            iconName: "folder"
            onClicked: ffmpegFolderDialog.open()
        }
        SButton {
            text: qsTr("Use System PATH")
            onClicked: app.clearCustomFfmpegDir()
        }
        Item { Layout.fillWidth: true }
    }

    Text {
        Layout.bottomMargin: Theme.s3
        text: app.ffmpegVersionText
        color: app.ffmpegCompatible ? Theme.good : Theme.warn
        font.family: Theme.monoFont
        font.pixelSize: Theme.fontSizeSmall
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.maximumWidth: 440
    }

    footerButtons: SButton {
        text: qsTr("Close")
        onClicked: dialog.close()
    }

    FolderDialog {
        id: ffmpegFolderDialog
        title: qsTr("Select FFmpeg folder")
        onAccepted: app.setCustomFfmpegDir(selectedFolder.toString())
    }
}
