import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Seder.VideoRewrap

// Project tab: media source, properties, output, project file, tools.
Flickable {
    id: panel

    contentWidth: width
    contentHeight: column.implicitHeight + Theme.s5 * 2
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ScrollBar.vertical: SScrollBar { }

    component SectionLabel: Text {
        Layout.topMargin: Theme.s3
        color: Theme.faint
        font.family: Theme.uiFont
        font.pixelSize: Theme.fontSizeTiny
        font.bold: true
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 1
    }

    component PropertyRow: RowLayout {
        property string label: ""
        property string value: ""
        property bool emphasize: false
        spacing: Theme.s3
        Layout.fillWidth: true
        Text {
            text: parent.label
            color: Theme.faint
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeSmall
            Layout.preferredWidth: 76
        }
        Text {
            text: parent.value
            color: parent.emphasize ? Theme.text : Theme.muted
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeSmall
            font.bold: parent.emphasize
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    component PathRow: Text {
        Layout.fillWidth: true
        color: Theme.muted
        font.family: Theme.monoFont
        font.pixelSize: Theme.fontSizeTiny
        elide: Text.ElideMiddle
    }

    ColumnLayout {
        id: column
        x: Theme.s4
        y: Theme.s4
        width: panel.width - Theme.s4 * 2
        spacing: Theme.s3

        SectionLabel { text: qsTr("Media"); Layout.topMargin: 0 }

        RowLayout {
            spacing: Theme.s3
            SButton {
                text: qsTr("Open Video…")
                iconName: "folder"
                tooltip: qsTr("Open a .mov, .mp4, or .mkv source file.")
                shortcutHint: "Ctrl+O"
                onClicked: app.openSource()
            }
            Item { Layout.fillWidth: true }
        }

        PathRow { text: app.sourcePath.length ? app.sourcePath : qsTr("No source selected") }

        PropertyRow { label: qsTr("Duration"); value: app.durationText; emphasize: true }
        PropertyRow { label: qsTr("Resolution"); value: app.resolutionText }
        PropertyRow { label: qsTr("Codec"); value: app.codecText }
        PropertyRow { label: qsTr("Size"); value: app.sizeText }
        PropertyRow { label: qsTr("Keyframes"); value: app.keyframeCount > 0 ? String(app.keyframeCount) : qsTr("N/A") }

        Text {
            text: app.mediaSummary
            color: Theme.faint
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeSmall
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        SectionLabel { text: qsTr("Output") }

        RowLayout {
            spacing: Theme.s3
            SButton {
                text: qsTr("Output File…")
                iconName: "save"
                tooltip: qsTr("Choose where the rewrapped file is written.")
                onClicked: app.chooseOutput()
            }
            Item { Layout.fillWidth: true }
        }

        PathRow { text: app.outputPath.length ? app.outputPath : qsTr("No output selected") }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        SectionLabel { text: qsTr("Project") }

        RowLayout {
            spacing: Theme.s3
            SButton {
                text: qsTr("Save")
                iconName: "save"
                shortcutHint: "Ctrl+S"
                tooltip: qsTr("Save segments and paths as a .seder-rewrap.json project.")
                Layout.fillWidth: true
                onClicked: app.saveProject()
            }
            SButton {
                text: qsTr("Load")
                iconName: "folder"
                shortcutHint: "Ctrl+Shift+O"
                tooltip: qsTr("Load a saved project.")
                Layout.fillWidth: true
                onClicked: app.loadProject()
            }
        }

        RowLayout {
            spacing: Theme.s3
            SButton {
                text: qsTr("TXT Report")
                iconName: "list"
                tooltip: qsTr("Write a human-readable segment report.")
                Layout.fillWidth: true
                onClicked: app.exportTxtReport()
            }
            SButton {
                text: qsTr("CSV Report")
                iconName: "list"
                tooltip: qsTr("Write segment data for spreadsheet tools.")
                Layout.fillWidth: true
                onClicked: app.exportCsvReport()
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        SectionLabel { text: qsTr("Tools") }

        RowLayout {
            spacing: Theme.s3
            SPill {
                text: app.ffmpegReady && app.ffprobeReady ? qsTr("FFmpeg Ready") : qsTr("FFmpeg Missing")
                tone: app.ffmpegReady && app.ffprobeReady ? Theme.good : Theme.bad
            }
            Item { Layout.fillWidth: true }
            SIconButton {
                iconName: "refresh"
                tooltip: qsTr("Recheck FFmpeg tools")
                onClicked: app.recheckTools()
            }
        }

        Text {
            text: app.ffmpegVersionText
            color: Theme.faint
            font.family: Theme.monoFont
            font.pixelSize: Theme.fontSizeTiny
            wrapMode: Text.Wrap
            Layout.fillWidth: true
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }

        SectionLabel { text: qsTr("Appearance") }

        RowLayout {
            spacing: Theme.s3
            SButton {
                text: qsTr("System")
                primary: app.theme === "system"
                Layout.fillWidth: true
                onClicked: app.setTheme("system")
            }
            SButton {
                text: qsTr("Light")
                primary: app.theme === "light"
                Layout.fillWidth: true
                onClicked: app.setTheme("light")
            }
            SButton {
                text: qsTr("Dark")
                primary: app.theme === "dark"
                Layout.fillWidth: true
                onClicked: app.setTheme("dark")
            }
        }
    }
}
