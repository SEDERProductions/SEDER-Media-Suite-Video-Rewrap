import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Seder.VideoRewrap

// Empty state: drop target, primary actions, and recent files.
Rectangle {
    id: welcome

    color: Theme.bg

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.s6 * 2, 560)
        spacing: Theme.s4

        SIcon {
            name: "film"
            width: 56
            height: 56
            color: Theme.border
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: qsTr("Drop a video to begin")
            color: Theme.text
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeTitle
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: qsTr("Build keyframe-aligned segments and export with FFmpeg stream copy — no re-encoding, no quality loss.")
            color: Theme.muted
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeBody
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        RowLayout {
            spacing: Theme.s3
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.s2

            SButton {
                text: qsTr("Open Video…")
                iconName: "folder"
                primary: true
                implicitHeight: 34
                shortcutHint: "Ctrl+O"
                onClicked: app.openSource()
            }
            SButton {
                text: qsTr("Load Project…")
                iconName: "save"
                implicitHeight: 34
                shortcutHint: "Ctrl+Shift+O"
                onClicked: app.loadProject()
            }
        }

        Item { Layout.preferredHeight: Theme.s4 }

        RowLayout {
            spacing: Theme.s6
            Layout.fillWidth: true
            visible: app.recentSources.count > 0 || app.recentProjects.count > 0

            // Recent videos
            ColumnLayout {
                spacing: Theme.s2
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                visible: app.recentSources.count > 0

                Text {
                    text: qsTr("Recent Videos")
                    color: Theme.faint
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeTiny
                    font.bold: true
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1
                }

                Repeater {
                    model: app.recentSources
                    delegate: ItemDelegate {
                        id: recentSource
                        Layout.fillWidth: true
                        implicitHeight: 24
                        hoverEnabled: true
                        onClicked: app.openSourcePath(model.path)
                        contentItem: Text {
                            text: model.display.length ? model.display : model.path
                            color: recentSource.hovered ? Theme.accentHover : Theme.muted
                            font.family: Theme.monoFont
                            font.pixelSize: Theme.fontSizeSmall
                            elide: Text.ElideMiddle
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: recentSource.hovered ? Theme.panelAlt : "transparent"
                            radius: Theme.radius
                        }
                    }
                }
            }

            // Recent projects
            ColumnLayout {
                spacing: Theme.s2
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                visible: app.recentProjects.count > 0

                Text {
                    text: qsTr("Recent Projects")
                    color: Theme.faint
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeTiny
                    font.bold: true
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1
                }

                Repeater {
                    model: app.recentProjects
                    delegate: ItemDelegate {
                        id: recentProject
                        Layout.fillWidth: true
                        implicitHeight: 24
                        hoverEnabled: true
                        onClicked: app.openProjectPath(model.path)
                        contentItem: Text {
                            text: model.display.length ? model.display : model.path
                            color: recentProject.hovered ? Theme.accentHover : Theme.muted
                            font.family: Theme.monoFont
                            font.pixelSize: Theme.fontSizeSmall
                            elide: Text.ElideMiddle
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: recentProject.hovered ? Theme.panelAlt : "transparent"
                            radius: Theme.radius
                        }
                    }
                }
            }
        }
    }
}
