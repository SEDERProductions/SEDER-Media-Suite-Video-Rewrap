import QtQuick
import QtQuick.Layouts
import Seder.VideoRewrap

SDialog {
    id: dialog

    titleText: qsTr("Keyboard Shortcuts")
    width: 560

    readonly property var groups: [
        {
            title: qsTr("Files"),
            items: [
                { keys: "Ctrl+O", action: qsTr("Open video") },
                { keys: "Ctrl+Shift+O", action: qsTr("Load project") },
                { keys: "Ctrl+S", action: qsTr("Save project") },
                { keys: "Ctrl+E", action: qsTr("Start export") }
            ]
        },
        {
            title: qsTr("Navigation"),
            items: [
                { keys: ",", action: qsTr("Previous keyframe") },
                { keys: ".", action: qsTr("Next keyframe") },
                { keys: "I", action: qsTr("Set IN marker") },
                { keys: "O", action: qsTr("Set OUT marker") }
            ]
        },
        {
            title: qsTr("Segments"),
            items: [
                { keys: qsTr("Up / Down"), action: qsTr("Select previous / next segment") },
                { keys: qsTr("Space"), action: qsTr("Toggle segment enabled") },
                { keys: qsTr("Delete"), action: qsTr("Remove selected segment") },
                { keys: "Ctrl+Z", action: qsTr("Undo") },
                { keys: "Ctrl+Shift+Z", action: qsTr("Redo") }
            ]
        }
    ]

    GridLayout {
        Layout.topMargin: Theme.s3
        Layout.bottomMargin: Theme.s3
        columns: 2
        columnSpacing: Theme.s6
        rowSpacing: Theme.s5

        Repeater {
            model: dialog.groups
            delegate: ColumnLayout {
                required property var modelData
                spacing: Theme.s2
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 240

                Text {
                    text: modelData.title
                    color: Theme.faint
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeTiny
                    font.bold: true
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 1
                }

                Repeater {
                    model: modelData.items
                    delegate: RowLayout {
                        required property var modelData
                        spacing: Theme.s3
                        Layout.fillWidth: true

                        Rectangle {
                            implicitWidth: keyText.implicitWidth + Theme.s3 * 2
                            implicitHeight: 20
                            radius: Theme.radius
                            color: Theme.panelAlt
                            border.color: Theme.border
                            border.width: 1

                            Text {
                                id: keyText
                                anchors.centerIn: parent
                                text: modelData.keys
                                color: Theme.text
                                font.family: Theme.monoFont
                                font.pixelSize: Theme.fontSizeTiny
                            }
                        }

                        Text {
                            text: modelData.action
                            color: Theme.muted
                            font.family: Theme.uiFont
                            font.pixelSize: Theme.fontSizeSmall
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }

    footerButtons: SButton {
        text: qsTr("Close")
        onClicked: dialog.close()
    }
}
