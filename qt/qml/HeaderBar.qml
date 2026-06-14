import QtQuick
import QtQuick.Layouts
import Seder.VideoRewrap

// Slim identity bar under the menu: brand, loaded source, tool status.
Rectangle {
    id: header

    implicitHeight: 34
    color: Theme.bg

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.s4
        anchors.rightMargin: Theme.s4
        spacing: Theme.s4

        Image {
            source: "qrc:/branding/icon-64.png"
            sourceSize: Qt.size(18, 18)
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
        }

        Text {
            text: qsTr("SEDER Video Rewrap")
            color: Theme.text
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeMedium
            font.bold: true
        }

        Text {
            text: app.appVersion
            color: Theme.faint
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeTiny
        }

        Item { Layout.fillWidth: true }

        Text {
            visible: app.sourcePath.length > 0
            text: app.mediaFilename
            color: Theme.muted
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeSmall
            elide: Text.ElideMiddle
            Layout.maximumWidth: header.width * 0.4
        }

        Item { Layout.fillWidth: true }

        SPill {
            text: app.ffmpegReady && app.ffprobeReady && app.ffmpegCompatible
                ? qsTr("FFmpeg Ready") : qsTr("FFmpeg Missing")
            iconName: app.ffmpegReady && app.ffprobeReady && app.ffmpegCompatible ? "check" : "alert"
            tone: app.ffmpegReady && app.ffprobeReady && app.ffmpegCompatible ? Theme.good : Theme.bad
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
