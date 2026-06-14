import QtQuick
import QtQuick.Layouts
import Seder.VideoRewrap

// Program monitor: the frame at the playhead, with transport below.
// Until the frame grabber lands, the viewport shows position context.
Rectangle {
    id: monitor

    color: Theme.panel
    border.color: Theme.border
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            color: Theme.headerStrip

            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.s3
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Program")
                color: Theme.muted
                font.family: Theme.uiFont
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.8
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: Theme.s3
                anchors.verticalCenter: parent.verticalCenter
                text: app.resolutionText !== "N/A" ? app.resolutionText : ""
                color: Theme.faint
                font.family: Theme.monoFont
                font.pixelSize: Theme.fontSizeTiny
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Theme.border
            }
        }

        // Viewport
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.dark ? "#0a0a0a" : "#1a1a1a"

            Image {
                id: frame
                anchors.fill: parent
                anchors.margins: Theme.s3
                fillMode: Image.PreserveAspectFit
                cache: false
                asynchronous: true
                source: app.frameGrabber.hasFrame
                    ? "image://frame/" + app.frameGrabber.frameSerial
                    : ""
                visible: app.frameGrabber.hasFrame
            }

            // Timecode overlay, bottom-left like a real program monitor.
            Rectangle {
                visible: app.frameGrabber.hasFrame
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                anchors.margins: Theme.s4
                width: overlayTc.implicitWidth + Theme.s4
                height: 22
                radius: Theme.radius
                color: "#b0000000"

                STimecodeLabel {
                    id: overlayTc
                    anchors.centerIn: parent
                    text: app.currentKeyframeText
                    color: "#e8e2dc"
                }
            }

            // Subtle activity bar while a grab is in flight.
            SProgressBar {
                visible: app.frameGrabber.grabbing
                indeterminate: true
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 2
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Theme.s3
                visible: !app.frameGrabber.hasFrame

                SIcon {
                    name: "film"
                    width: 40
                    height: 40
                    color: "#3a3a3a"
                    Layout.alignment: Qt.AlignHCenter
                }

                STimecodeLabel {
                    emphasized: true
                    text: app.currentKeyframeText
                    color: "#9e9e9e"
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: app.keyframeCount > 0
                        ? qsTr("Use the transport or , and . to step keyframes")
                        : qsTr("Open a video to inspect keyframes")
                    color: "#5c5c5c"
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeSmall
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        TransportBar {
            Layout.fillWidth: true
        }
    }
}
