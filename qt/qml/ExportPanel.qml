import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Seder.VideoRewrap

// Export tab: mode cards, the big primary action, and progress.
Flickable {
    id: panel

    contentWidth: width
    contentHeight: column.implicitHeight + Theme.s5 * 2
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ScrollBar.vertical: SScrollBar { }

    component ModeCard: Rectangle {
        id: card
        property string mode: ""
        property string title: ""
        property string description: ""
        property string iconName: ""
        readonly property bool active: app.exportMode === mode

        Layout.fillWidth: true
        implicitHeight: cardColumn.implicitHeight + Theme.s4 * 2
        radius: Theme.radiusLarge
        color: active ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.dark ? 0.12 : 0.08)
            : cardMouse.containsMouse ? Theme.controlBgHover : Theme.panelAlt
        border.color: active ? Theme.accent : Theme.border
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }

        Accessible.role: Accessible.RadioButton
        Accessible.name: title

        ColumnLayout {
            id: cardColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: Theme.s4
            spacing: Theme.s2

            RowLayout {
                spacing: Theme.s3
                SIcon {
                    name: card.iconName
                    width: 14
                    height: 14
                    color: card.active ? Theme.accentHover : Theme.muted
                }
                Text {
                    text: card.title
                    color: Theme.text
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeBody
                    font.bold: true
                    Layout.fillWidth: true
                }
                SIcon {
                    name: "check"
                    width: 12
                    height: 12
                    color: Theme.accentHover
                    visible: card.active
                }
            }

            Text {
                text: card.description
                color: Theme.muted
                font.family: Theme.uiFont
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        MouseArea {
            id: cardMouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: app.setExportMode(card.mode)
        }
    }

    ColumnLayout {
        id: column
        x: Theme.s4
        y: Theme.s4
        width: panel.width - Theme.s4 * 2
        spacing: Theme.s3

        Text {
            text: qsTr("Export Mode")
            color: Theme.faint
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeTiny
            font.bold: true
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 1
        }

        ModeCard {
            mode: "concat_single"
            title: qsTr("Single File")
            iconName: "film"
            description: qsTr("All enabled segments joined into one output file. Stream copy, no re-encode.")
        }

        ModeCard {
            mode: "separate_files"
            title: qsTr("Separate Files")
            iconName: "duplicate"
            description: qsTr("Each enabled segment exported as its own file, numbered in order.")
        }

        Item { Layout.preferredHeight: Theme.s2 }

        SButton {
            text: app.busy ? qsTr("Exporting…") : qsTr("Start Export")
            iconName: "upload"
            primary: true
            enabled: !app.busy
            implicitHeight: 36
            shortcutHint: "Ctrl+E"
            tooltip: qsTr("Run FFmpeg stream-copy export for all enabled segments.")
            Layout.fillWidth: true
            onClicked: app.startExport()
        }

        SProgressBar {
            value: app.progress
            visible: app.busy
            Layout.fillWidth: true
        }

        SButton {
            text: qsTr("Cancel Export")
            iconName: "close"
            visible: app.busy
            Layout.fillWidth: true
            onClicked: app.cancelExport()
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        SButton {
            text: qsTr("Replace Original File")
            iconName: "swap"
            enabled: !app.busy && app.sourcePath.length > 0
            tooltip: qsTr("Renames the source to _PREWRAP and exports the new file in its place.")
            Layout.fillWidth: true
            onClicked: app.replaceFile()
        }
    }
}
