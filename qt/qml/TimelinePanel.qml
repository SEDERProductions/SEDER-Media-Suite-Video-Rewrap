import QtQuick
import QtQuick.Layouts
import Seder.VideoRewrap

// Timeline tab: marker readouts + zoom controls above the timeline view.
Item {
    id: panel

    function zoomIn() { timeline.zoomAt(timeline.width / 2, 1.25) }
    function zoomOut() { timeline.zoomAt(timeline.width / 2, 0.8) }
    function fit() { timeline.fit() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Theme.panel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.s3
                anchors.rightMargin: Theme.s3
                spacing: Theme.s3

                SIcon {
                    name: "markIn"
                    width: 12
                    height: 12
                    color: app.hasPendingIn ? Theme.accentHover : Theme.faint
                }
                STimecodeLabel {
                    text: app.pendingInText
                    Accessible.name: qsTr("Pending IN %1").arg(text)
                }
                SIcon {
                    name: "markOut"
                    width: 12
                    height: 12
                    color: app.hasPendingOut ? Theme.accentHover : Theme.faint
                }
                STimecodeLabel {
                    text: app.pendingOutText
                    Accessible.name: qsTr("Pending OUT %1").arg(text)
                }
                SIconButton {
                    iconName: "close"
                    iconSize: 10
                    tooltip: qsTr("Clear IN/OUT markers")
                    visible: app.hasPendingIn || app.hasPendingOut
                    onClicked: app.clearPendingMarkers()
                }

                Item { Layout.fillWidth: true }

                Text {
                    id: segmentCount
                    // rowCount() is a plain method; `tick` forces the
                    // binding to re-run whenever segments mutate.
                    property int tick: 0
                    text: qsTr("%1 segments").arg(tick >= 0 ? segmentModel.rowCount() : 0)
                    color: Theme.faint
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeTiny
                    Connections {
                        target: app
                        function onSegmentsChanged() { segmentCount.tick++ }
                    }
                }

                SIconButton {
                    iconName: "zoomOut"
                    tooltip: qsTr("Zoom out")
                    shortcutHint: "-"
                    onClicked: panel.zoomOut()
                }

                SSlider {
                    id: zoomSlider
                    Layout.preferredWidth: 110
                    from: 0
                    to: 1
                    value: timeline.zoomT
                    onMoved: timeline.setZoomT(value)
                    Accessible.name: qsTr("Timeline zoom")
                }

                SIconButton {
                    iconName: "zoomIn"
                    tooltip: qsTr("Zoom in")
                    shortcutHint: "+"
                    onClicked: panel.zoomIn()
                }

                SIconButton {
                    iconName: "fit"
                    tooltip: qsTr("Fit timeline to window")
                    onClicked: panel.fit()
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

        TimelineView {
            id: timeline
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
