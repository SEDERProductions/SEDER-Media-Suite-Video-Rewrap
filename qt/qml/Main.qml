import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    width: Math.max(minimumWidth, Math.min(maximumWidth, Screen.width * 0.86))
    height: Math.max(minimumHeight, Math.min(maximumHeight, Screen.height * 0.9))
    minimumWidth: 980
    minimumHeight: 700
    maximumWidth: 1920
    maximumHeight: 1280
    visible: true
    title: "SEDER Media Suite Video Rewrap"


    readonly property bool compactLayout: width < 1240
    readonly property real leftRailWidth: Math.max(72, Math.min(96, width * 0.07))
    readonly property real toolsPanePreferredWidth: Math.max(300, Math.min(420, width * (compactLayout ? 0.3 : 0.27)))
    readonly property real tableAvailableWidth: Math.max(620, table.width)
    readonly property var tableColumnMinimums: [64, 170, 120, 120, 120, 220]
    readonly property var tableColumnWeights: [1.0, 2.0, 1.3, 1.3, 1.3, 3.1]

    property var __cachedColumnWidths: []

    function recalculateColumnWidths() {
        var minimumTotal = 0
        for (var i = 0; i < tableColumnMinimums.length; ++i)
            minimumTotal += tableColumnMinimums[i]

        var extra = Math.max(0, tableAvailableWidth - minimumTotal)
        var weightTotal = 0
        for (var j = 0; j < tableColumnWeights.length; ++j)
            weightTotal += tableColumnWeights[j]

        var widths = []
        for (var k = 0; k < tableColumnMinimums.length; ++k) {
            widths[k] = tableColumnMinimums[k] + extra * (tableColumnWeights[k] / weightTotal)
        }

        var notesIndex = tableColumnMinimums.length - 1
        var used = 0
        for (var n = 0; n < notesIndex; ++n)
            used += widths[n]
        widths[notesIndex] = Math.max(tableColumnMinimums[notesIndex], tableAvailableWidth - used)

        __cachedColumnWidths = widths
    }

    onTableAvailableWidthChanged: recalculateColumnWidths()

    function tableColumnWidth(column) {
        return __cachedColumnWidths[column] || 0
    }
    readonly property bool dark: app.darkMode
    readonly property color bg: dark ? "#12110f" : "#ece6d9"
    readonly property color panel: dark ? "#1f1d1a" : "#f8f4ea"
    readonly property color panelAlt: dark ? "#302c27" : "#d8d0be"
    readonly property color ink: dark ? "#ece6d9" : "#16140f"
    readonly property color muted: dark ? "#c8bfaf" : "#3f392f"
    readonly property color faint: dark ? "#978d7d" : "#5f584c"
    readonly property color line: dark ? "#5a5247" : "#b8af9c"
    readonly property color accent: "#c63b13"
    readonly property color good: dark ? "#67c89a" : "#16663f"
    readonly property color warn: dark ? "#e2b667" : "#85580b"
    readonly property color bad: dark ? "#e37a6b" : "#963018"
    readonly property string uiFontFamily: Qt.application.font.family
    readonly property string monoFontFamily: Qt.platform.os === "osx"
        ? "Menlo"
        : (Qt.platform.os === "windows" ? "Consolas" : "Monospace")

    color: bg

    component MetaLabel: Label {
        color: faint
        font.family: root.monoFontFamily
        font.pixelSize: 10
        font.capitalization: Font.AllUppercase
        elide: Text.ElideRight
    }

    component PathLabel: Label {
        color: muted
        font.family: root.monoFontFamily
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
        property string stateIcon: "\u2022"
        implicitHeight: 24
        implicitWidth: label.implicitWidth + 20
        radius: 12
        color: Qt.rgba(tone.r, tone.g, tone.b, root.dark ? 0.22 : 0.16)
        border.color: Qt.rgba(tone.r, tone.g, tone.b, root.dark ? 0.9 : 0.72)
        border.width: 1
        Label {
            id: label
            anchors.centerIn: parent
            text: statusPill.stateIcon + " " + statusPill.text
            color: statusPill.tone
            font.family: root.monoFontFamily
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
        font.family: root.uiFontFamily
        palette.buttonText: primary ? "white" : (commandButton.enabled ? root.ink : root.muted)
        opacity: commandButton.enabled ? 1.0 : 0.62
        background: Rectangle {
            radius: 5
            color: commandButton.enabled
                ? (commandButton.primary ? root.accent : root.panelAlt)
                : (root.dark ? "#2a2620" : "#ddd4c1")
            border.color: commandButton.enabled
                ? (commandButton.primary ? "transparent" : root.line)
                : root.line
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
                font.family: root.uiFontFamily
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
        font.family: root.uiFontFamily
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
            Layout.preferredHeight: compactLayout ? 72 : 86
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
                        font.family: root.uiFontFamily
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
                Layout.preferredWidth: leftRailWidth
                Layout.minimumWidth: 72
                Layout.maximumWidth: 96
                Layout.fillWidth: true
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
                            font.family: root.uiFontFamily
                            font.bold: true
                            font.pixelSize: 18
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            ScrollView {
                Layout.preferredWidth: toolsPanePreferredWidth
                Layout.minimumWidth: 280
                Layout.maximumWidth: 460
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                background: Rectangle { color: panel; border.color: line }
                ColumnLayout {
                    width: parent.availableWidth - 28
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

                    MetaLabel { text: "04 / EXPORT" }
                    RowLayout {
                        Layout.fillWidth: true
                        CommandButton { text: "Concat Single"; Layout.fillWidth: true; onClicked: app.setExportMode("concat_single") }
                        CommandButton { text: "Separate Files"; Layout.fillWidth: true; onClicked: app.setExportMode("separate_files") }
                    }
                    PathLabel { text: "Mode: " + app.exportMode; Layout.fillWidth: true }
                    CommandButton {
                        text: app.busy ? "Exporting..." : "Start Export"
                        primary: true
                        enabled: !app.busy
                        Layout.fillWidth: true
                        onClicked: app.startExport()
                    }
                    CommandButton {
                        text: "Replace File"
                        enabled: !app.busy && app.sourcePath.length > 0
                        Layout.fillWidth: true
                        onClicked: app.replaceFile()
                        ToolTip.visible: hovered
                        ToolTip.text: "Renames source to _PREWRAP and exports new file in its place."
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
                Layout.minimumWidth: 460
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
                        implicitHeight: compactLayout ? 154 : 134
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
                        implicitHeight: compactLayout ? 242 : 210
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
                        implicitHeight: Math.max(compactLayout ? 300 : 340, root.height - (compactLayout ? 500 : 536))
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
                                        width: root.tableColumnWidth(index)
                                        implicitHeight: 28
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
                                columnWidthProvider: function(column) { return root.tableColumnWidth(column) }
                                rowHeightProvider: function(row) { return 34 }
                                delegate: Rectangle {
                                    required property int row
                                    required property int column
                                    required property string display
                                    color: row === app.selectedRow ? (root.dark ? "#3f251d" : "#f2d6c8")
                                                                    : (row % 2 === 0 ? root.panel : root.panelAlt)
                                    border.color: row === app.selectedRow ? root.accent : root.line
                                    border.width: row === app.selectedRow ? 2 : 1
                                    Rectangle {
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.bottom: parent.bottom
                                        width: row === app.selectedRow ? 5 : 0
                                        color: root.accent
                                        visible: row === app.selectedRow
                                    }
                                    Text {
                                        anchors.fill: parent
                                        anchors.leftMargin: row === app.selectedRow ? 12 : 8
                                        anchors.rightMargin: 8
                                        verticalAlignment: Text.AlignVCenter
                                        text: display
                                        color: column === 0 && display === "OFF" ? root.faint : root.ink
                                        font.family: column >= 2 ? root.monoFontFamily : root.uiFontFamily
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
            Layout.preferredHeight: compactLayout ? 42 : 48
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
