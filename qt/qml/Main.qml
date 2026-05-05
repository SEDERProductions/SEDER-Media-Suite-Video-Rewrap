import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1380
    height: 920
    minimumWidth: 1060
    minimumHeight: 760
    visible: true
    title: "SEDER Media Suite Video Rewrap"

    readonly property bool dark: app.darkMode
    readonly property color bg: dark ? "#12110f" : "#ece6d9"
    readonly property color panel: dark ? "#1f1d1a" : "#f8f4ea"
    readonly property color panelAlt: dark ? "#282521" : "#e3dccb"
    readonly property color ink: dark ? "#ece6d9" : "#16140f"
    readonly property color muted: dark ? "#ada596" : "#4a4438"
    readonly property color faint: dark ? "#716a5f" : "#7a7363"
    readonly property color line: dark ? "#3a352d" : "#d6cfbe"
    readonly property color accent: "#c63b13"
    readonly property color good: dark ? "#4cab7e" : "#1f7a4d"
    readonly property color warn: dark ? "#c99746" : "#9a6a16"
    readonly property color bad: dark ? "#d25645" : "#b43a1f"

    color: bg

    component MetaLabel: Label {
        color: faint
        font.family: "Menlo"
        font.pixelSize: 10
        font.capitalization: Font.AllUppercase
        elide: Text.ElideRight
    }

    component PathLabel: Label {
        color: muted
        font.family: "Menlo"
        font.pixelSize: 12
        elide: Text.ElideMiddle
        wrapMode: Text.NoWrap
    }

    component Panel: Rectangle {
        color: root.panel
        border.color: root.line
        border.width: 1
        radius: 6
    }

    component StatusPill: Rectangle {
        id: statusPill
        property string text: ""
        property color tone: root.faint
        implicitHeight: 24
        implicitWidth: label.implicitWidth + 18
        radius: 12
        color: Qt.rgba(tone.r, tone.g, tone.b, 0.12)
        border.color: Qt.rgba(tone.r, tone.g, tone.b, 0.55)
        border.width: 1
        Label {
            id: label
            anchors.centerIn: parent
            text: "\u25cf " + statusPill.text
            color: statusPill.tone
            font.family: "Menlo"
            font.pixelSize: 10
            font.capitalization: Font.AllUppercase
        }
    }

    component CommandButton: Button {
        id: commandButton
        property bool primary: false
        implicitHeight: 34
        padding: 10
        font.pixelSize: 13
        palette.buttonText: primary ? "white" : root.ink
        background: Rectangle {
            radius: 5
            color: commandButton.enabled
                ? (commandButton.primary ? root.accent : root.panelAlt)
                : Qt.rgba(root.faint.r, root.faint.g, root.faint.b, 0.18)
            border.color: commandButton.primary ? "transparent" : root.line
            border.width: commandButton.primary ? 0 : 1
        }
    }

    component Metric: Panel {
        id: metric
        property string label: ""
        property string value: ""
        property color tone: root.ink
        implicitHeight: 72
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4
            MetaLabel { text: metric.label }
            Label {
                text: metric.value
                color: metric.tone
                font.bold: true
                font.pixelSize: 18
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    component Field: TextField {
        color: root.ink
        placeholderTextColor: root.faint
        selectedTextColor: "white"
        selectionColor: root.accent
        font.pixelSize: 13
        background: Rectangle {
            radius: 5
            color: root.dark ? "#181613" : "#fffaf0"
            border.color: root.line
            border.width: 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 86
            color: bg
            border.color: line
            border.width: 0
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: 14
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Label {
                        text: "SEDER Media Suite Video Rewrap"
                        color: ink
                        font.pixelSize: 30
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    MetaLabel { text: "VOL. 04 / KEYFRAME STREAM COPY" }
                }
                StatusPill {
                    text: app.ffmpegReady && app.ffprobeReady ? "READY" : "MISSING"
                    tone: app.ffmpegReady && app.ffprobeReady ? good : bad
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 88
                Layout.fillHeight: true
                color: panel
                border.color: line
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10
                    StatusPill { text: "REC"; tone: accent; Layout.alignment: Qt.AlignHCenter }
                    Rectangle { Layout.fillWidth: true; height: 1; color: line }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 62
                        radius: 5
                        color: panelAlt
                        border.color: accent
                        Label {
                            anchors.centerIn: parent
                            text: "RW"
                            color: ink
                            font.bold: true
                            font.pixelSize: 18
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            ScrollView {
                Layout.preferredWidth: 372
                Layout.fillHeight: true
                clip: true
                background: Rectangle { color: panel; border.color: line }
                ColumnLayout {
                    width: 348
                    x: 14
                    y: 14
                    spacing: 14

                    MetaLabel { text: "01 / TOOLS" }
                    RowLayout {
                        CommandButton { text: "Recheck FFmpeg"; onClicked: app.recheckTools() }
                        StatusPill {
                            text: app.ffmpegReady && app.ffprobeReady ? "READY" : "MISSING"
                            tone: app.ffmpegReady && app.ffprobeReady ? good : bad
                        }
                    }
                    PathLabel { text: app.logText; Layout.fillWidth: true; wrapMode: Text.Wrap }

                    MetaLabel { text: "02 / FILES" }
                    CommandButton { text: "Open Video"; Layout.fillWidth: true; onClicked: app.openSource() }
                    PathLabel { text: app.sourcePath.length ? app.sourcePath : "No source selected"; Layout.fillWidth: true }
                    CommandButton { text: "Output File"; Layout.fillWidth: true; onClicked: app.chooseOutput() }
                    PathLabel { text: app.outputPath.length ? app.outputPath : "No output selected"; Layout.fillWidth: true }

                    MetaLabel { text: "03 / PROJECT" }
                    RowLayout {
                        CommandButton { text: "Save"; Layout.fillWidth: true; onClicked: app.saveProject() }
                        CommandButton { text: "Load"; Layout.fillWidth: true; onClicked: app.loadProject() }
                    }
                    RowLayout {
                        CommandButton { text: "TXT Report"; Layout.fillWidth: true; onClicked: app.exportTxtReport() }
                        CommandButton { text: "CSV Report"; Layout.fillWidth: true; onClicked: app.exportCsvReport() }
                    }

                    MetaLabel { text: "04 / EXPORT VIDEO" }
                    MetaLabel { text: "Tip: Export requires FFmpeg/FFprobe, a source video, and an output file path." }
                    CommandButton {
                        text: app.busy ? "Exporting..." : "Start Export"
                        primary: true
                        enabled: !app.busy
                        Layout.fillWidth: true
                        onClicked: app.startExport()
                    }
                    CommandButton {
                        text: "Cancel Export"
                        enabled: app.busy
                        Layout.fillWidth: true
                        onClicked: app.cancelExport()
                    }
                    ProgressBar {
                        from: 0
                        to: 1
                        value: app.progress
                        Layout.fillWidth: true
                    }

                    MetaLabel { text: "05 / THEME" }
                    RowLayout {
                        CommandButton { text: "System"; Layout.fillWidth: true; onClicked: app.setTheme("system") }
                        CommandButton { text: "Light"; Layout.fillWidth: true; onClicked: app.setTheme("light") }
                        CommandButton { text: "Dark"; Layout.fillWidth: true; onClicked: app.setTheme("dark") }
                    }
                }
            }

            Flickable {
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: width
                contentHeight: workspace.implicitHeight + 24
                clip: true

                ColumnLayout {
                    id: workspace
                    x: 14
                    y: 14
                    width: parent.width - 28
                    spacing: 12

                    Panel {
                        Layout.fillWidth: true
                        implicitHeight: 134
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10
                            MetaLabel { text: "MEDIA METADATA" }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 4
                                columnSpacing: 10
                                Metric { label: "Duration"; value: app.durationText; Layout.fillWidth: true }
                                Metric { label: "Resolution"; value: app.resolutionText; Layout.fillWidth: true }
                                Metric { label: "Codec"; value: app.codecText; Layout.fillWidth: true }
                                Metric { label: "Size"; value: app.sizeText; Layout.fillWidth: true }
                            }
                            PathLabel { text: app.mediaSummary; Layout.fillWidth: true }
                        }
                    }

                    Panel {
                        Layout.fillWidth: true
                        implicitHeight: 210
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10
                            MetaLabel { text: "KEYFRAME NAVIGATION AND SEGMENT CREATION" }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 3
                                columnSpacing: 10
                                Metric { label: "Keyframes"; value: app.keyframeCount.toString(); Layout.fillWidth: true }
                                Metric { label: "Current"; value: app.currentKeyframeText; tone: good; Layout.fillWidth: true }
                                Metric { label: "Selected"; value: app.totalDurationText; Layout.fillWidth: true }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                CommandButton { text: "Go to Previous Keyframe"; onClicked: app.previousKeyframe(); ToolTip.visible: hovered; ToolTip.text: "Move playhead to the previous detected keyframe." }
                                CommandButton { text: "Go to Next Keyframe"; onClicked: app.nextKeyframe(); ToolTip.visible: hovered; ToolTip.text: "Move playhead to the next detected keyframe." }
                                CommandButton { text: "Preview Selected Keyframe"; onClicked: app.previewCurrent(); ToolTip.visible: hovered; ToolTip.text: "Play a short preview at the selected keyframe." }
                                MetaLabel { text: "JUMP TO TIMECODE"; Layout.leftMargin: 8 }
                                Field {
                                    id: jumpTime
                                    text: "00:00:00.000"
                                    Layout.preferredWidth: 150
                                }
                                CommandButton { text: "Jump to Timecode"; onClicked: app.jumpToTimecode(jumpTime.text); ToolTip.visible: hovered; ToolTip.text: "Use HH:MM:SS.mmm format to move to an exact time." }
                            }
                            MetaLabel { text: "Tip: Set IN and OUT markers before creating a segment." }
                            RowLayout {
                                Layout.fillWidth: true
                                CommandButton { text: "Set In Marker"; onClicked: app.setIn(); ToolTip.visible: hovered; ToolTip.text: "Set the start boundary for the segment you are creating." }
                                StatusPill { text: "IN " + app.pendingInText }
                                CommandButton { text: "Set Out Marker"; onClicked: app.setOut(); ToolTip.visible: hovered; ToolTip.text: "Set the end boundary for the segment you are creating." }
                                StatusPill { text: "OUT " + app.pendingOutText }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Field {
                                    id: segmentName
                                    placeholderText: "Segment name"
                                    Layout.preferredWidth: 180
                                }
                                Field {
                                    id: segmentNotes
                                    placeholderText: "Notes"
                                    Layout.fillWidth: true
                                }
                                CommandButton {
                                    text: "Create Segment"
                                    primary: true
                                    onClicked: {
                                        app.addSegment(segmentName.text, segmentNotes.text)
                                        segmentName.text = ""
                                        segmentNotes.text = ""
                                    }
                                }
                            }
                        }
                    }

                    Panel {
                        Layout.fillWidth: true
                        implicitHeight: Math.max(340, root.height - 536)
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10
                            RowLayout {
                                Layout.fillWidth: true
                                MetaLabel { text: "SEGMENT LIST"; Layout.fillWidth: true }
                                StatusPill { text: "TOTAL " + app.totalDurationText; tone: good }
                            }
                            Row {
                                Layout.fillWidth: true
                                height: 28
                                Repeater {
                                    model: ["Enabled", "Segment Name", "In Marker", "Out Marker", "Duration", "Notes"]
                                    Rectangle {
                                        width: [72, 210, 132, 132, 132, 320][index]
                                        height: 28
                                        color: panelAlt
                                        border.color: line
                                        MetaLabel {
                                            anchors.verticalCenter: parent.verticalCenter
                                            anchors.left: parent.left
                                            anchors.leftMargin: 8
                                            text: modelData
                                        }
                                    }
                                }
                            }
                            TableView {
                                id: table
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                model: segmentModel
                                columnWidthProvider: function(column) { return [72, 210, 132, 132, 132, 320][column] }
                                rowHeightProvider: function(row) { return 34 }
                                delegate: Rectangle {
                                    required property int row
                                    required property int column
                                    required property string display
                                    color: row === app.selectedRow ? Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.16)
                                                                    : (row % 2 === 0 ? root.panel : root.panelAlt)
                                    border.color: root.line
                                    Text {
                                        anchors.fill: parent
                                        anchors.leftMargin: 8
                                        anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: display
                                        color: column === 0 && display === "OFF" ? root.faint : root.ink
                                        font.family: column >= 2 ? "Menlo, Consolas, monospace" : "Manrope"
                                        font.pixelSize: 12
                                        elide: Text.ElideRight
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: app.selectSegment(row)
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                CommandButton { text: "Set Enabled"; enabled: app.selectedRow >= 0; onClicked: app.toggleSegment(app.selectedRow, true) }
                                CommandButton { text: "Set Disabled"; enabled: app.selectedRow >= 0; onClicked: app.toggleSegment(app.selectedRow, false) }
                                CommandButton { text: "Move Up"; enabled: app.selectedRow > 0; onClicked: app.moveSegmentUp(app.selectedRow) }
                                CommandButton { text: "Move Down"; enabled: app.selectedRow >= 0 && app.selectedRow + 1 < segmentModel.rowCount(); onClicked: app.moveSegmentDown(app.selectedRow) }
                                CommandButton { text: "Duplicate"; enabled: app.selectedRow >= 0; onClicked: app.duplicateSegment(app.selectedRow) }
                                CommandButton { text: "Delete"; enabled: app.selectedRow >= 0; onClicked: app.removeSegment(app.selectedRow) }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: panel
            border.color: line
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 10
                StatusPill {
                    text: app.busy ? "WORKING" : "LOG"
                    tone: app.busy ? warn : faint
                }
                PathLabel {
                    text: app.logText
                    Layout.fillWidth: true
                }
            }
        }
    }
}
