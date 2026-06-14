import QtQuick
import QtQuick.Layouts
import Seder.VideoRewrap

// Bottom status strip: activity, log line, selection totals, keyframes.
Rectangle {
    id: statusBar

    implicitHeight: 26
    color: Theme.panel

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.border
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.s4
        anchors.rightMargin: Theme.s4
        spacing: Theme.s4

        SPill {
            text: app.busy ? qsTr("Working") : qsTr("Ready")
            tone: app.busy ? Theme.warn : Theme.faint
        }

        SProgressBar {
            visible: app.busy
            value: app.progress
            Layout.preferredWidth: 120
        }

        Text {
            text: app.logText
            color: Theme.muted
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeTiny
            elide: Text.ElideMiddle
            Layout.fillWidth: true

            Accessible.role: Accessible.StaticText
            Accessible.name: text
        }

        Text {
            visible: app.keyframeCount > 0
            text: qsTr("%1 keyframes").arg(app.keyframeCount)
            color: Theme.faint
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeTiny
        }

        Text {
            text: qsTr("Selected %1").arg(app.totalDurationText)
            color: Theme.faint
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeTiny
        }
    }
}
