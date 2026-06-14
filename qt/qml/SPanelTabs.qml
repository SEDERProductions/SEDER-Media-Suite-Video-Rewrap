import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Seder.VideoRewrap

// Premiere-style tabbed panel: tab strip in the header, one page visible.
// Declare pages as children; name them via `titles`.
Rectangle {
    id: root

    property var titles: []
    property int currentIndex: 0
    default property alias pages: stack.data

    color: Theme.panel
    border.color: Theme.border
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            color: Theme.headerStrip

            Row {
                anchors.left: parent.left
                anchors.leftMargin: Theme.s1
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                spacing: 0

                Repeater {
                    model: root.titles
                    delegate: Item {
                        required property int index
                        required property string modelData
                        readonly property bool active: root.currentIndex === index
                        width: tabLabel.implicitWidth + Theme.s5 * 2
                        height: parent.height

                        Rectangle {
                            anchors.fill: parent
                            color: parent.active ? Theme.panel
                                : tabMouse.containsMouse ? Qt.rgba(1, 1, 1, Theme.dark ? 0.04 : 0.25)
                                : "transparent"
                            Behavior on color { ColorAnimation { duration: Theme.durFast } }
                        }

                        Text {
                            id: tabLabel
                            anchors.centerIn: parent
                            text: modelData
                            color: parent.active ? Theme.text : Theme.muted
                            font.family: Theme.uiFont
                            font.pixelSize: Theme.fontSizeSmall
                            font.bold: parent.active
                            font.capitalization: Font.AllUppercase
                            font.letterSpacing: 0.8
                            Behavior on color { ColorAnimation { duration: Theme.durFast } }
                        }

                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 2
                            color: Theme.accent
                            visible: parent.active
                        }

                        MouseArea {
                            id: tabMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            onClicked: root.currentIndex = index
                        }

                        Accessible.role: Accessible.PageTab
                        Accessible.name: modelData
                    }
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

        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentIndex
        }
    }
}
