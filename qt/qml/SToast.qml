import QtQuick
import Seder.VideoRewrap

// One transient notification card. Hosted and stacked by ToastHost.
Rectangle {
    id: toast

    property string message: ""
    property string tone: "info" // "info" | "success" | "error"
    signal dismissed()

    readonly property color toneColor: tone === "success" ? Theme.good
        : tone === "error" ? Theme.bad : Theme.info

    implicitWidth: Math.min(row.implicitWidth + Theme.s5 * 2, 380)
    implicitHeight: Math.max(36, row.implicitHeight + Theme.s4)
    radius: Theme.radiusLarge
    color: Theme.dark ? "#101010" : "#ffffff"
    border.color: Qt.rgba(toneColor.r, toneColor.g, toneColor.b, 0.7)
    border.width: 1

    Accessible.role: Accessible.AlertMessage
    Accessible.name: message

    opacity: 0
    Component.onCompleted: opacity = 1
    Behavior on opacity { NumberAnimation { duration: Theme.durSlow } }

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 3
        radius: 1.5
        color: toast.toneColor
    }

    Row {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: Theme.s4
        anchors.right: parent.right
        anchors.rightMargin: Theme.s4
        spacing: Theme.s3

        SIcon {
            anchors.verticalCenter: parent.verticalCenter
            name: toast.tone === "success" ? "check" : toast.tone === "error" ? "alert" : "info"
            width: 13
            height: 13
            color: toast.toneColor
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, 320)
            text: toast.message
            color: Theme.text
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeBody
            wrapMode: Text.WordWrap
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: toast.dismissed()
    }

    Timer {
        interval: 4000
        running: true
        onTriggered: toast.dismissed()
    }
}
