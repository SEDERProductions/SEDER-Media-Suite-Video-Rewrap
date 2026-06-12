import QtQuick
import QtQuick.Layouts
import Seder.VideoRewrap

// Monitor transport: keyframe stepping, timecode jump, IN/OUT marking.
Rectangle {
    id: transport

    implicitHeight: 40
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
        anchors.leftMargin: Theme.s3
        anchors.rightMargin: Theme.s3
        spacing: Theme.s2

        SIconButton {
            iconName: "skipPrev"
            tooltip: qsTr("Previous keyframe")
            shortcutHint: ","
            enabled: app.keyframeCount > 0
            onClicked: app.previousKeyframe()
        }

        SIconButton {
            iconName: "skipNext"
            tooltip: qsTr("Next keyframe")
            shortcutHint: "."
            enabled: app.keyframeCount > 0
            onClicked: app.nextKeyframe()
        }

        SIconButton {
            iconName: "play"
            tooltip: qsTr("Preview from here in FFplay (with audio)")
            enabled: app.keyframeCount > 0 && app.ffplayReady
            onClicked: app.previewCurrent()
        }

        Rectangle { width: 1; Layout.fillHeight: true; Layout.topMargin: 8; Layout.bottomMargin: 8; color: Theme.border }

        SField {
            id: jumpField
            mono: true
            text: "00:00:00.000"
            Layout.preferredWidth: 110
            Accessible.name: qsTr("Jump to timecode")
            onAccepted: app.jumpToTimecode(text)
        }

        SIconButton {
            iconName: "chevronRight"
            tooltip: qsTr("Jump to timecode (HH:MM:SS.mmm)")
            enabled: app.keyframeCount > 0
            onClicked: app.jumpToTimecode(jumpField.text)
        }

        Item { Layout.fillWidth: true }

        SIconButton {
            iconName: "markIn"
            tooltip: qsTr("Set IN marker at the current keyframe")
            shortcutHint: "I"
            accented: app.pendingInText !== "-"
            enabled: app.keyframeCount > 0
            onClicked: app.setIn()
        }

        STimecodeLabel {
            text: app.pendingInText
            Accessible.name: qsTr("Pending IN marker %1").arg(text)
        }

        SIconButton {
            iconName: "markOut"
            tooltip: qsTr("Set OUT marker at the current keyframe")
            shortcutHint: "O"
            accented: app.pendingOutText !== "-"
            enabled: app.keyframeCount > 0
            onClicked: app.setOut()
        }

        STimecodeLabel {
            text: app.pendingOutText
            Accessible.name: qsTr("Pending OUT marker %1").arg(text)
        }

        Rectangle { width: 1; Layout.fillHeight: true; Layout.topMargin: 8; Layout.bottomMargin: 8; color: Theme.border }

        STimecodeLabel {
            emphasized: true
            text: app.currentKeyframeText
            Accessible.name: qsTr("Playhead position %1").arg(text)
        }
    }
}
