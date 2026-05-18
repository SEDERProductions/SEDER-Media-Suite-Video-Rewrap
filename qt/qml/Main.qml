import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
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
    title: qsTr("SEDER Media Suite Video Rewrap")


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
        property string stateIcon: "•"
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
        Accessible.role: Accessible.Button
        Accessible.name: text
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

    // ---- Global keyboard shortcuts ----
    Shortcut { sequence: StandardKey.Open;          onActivated: app.openSource() }
    Shortcut { sequence: "Ctrl+Shift+O";            onActivated: app.loadProject() }
    Shortcut { sequence: StandardKey.Save;          onActivated: app.saveProject() }
    Shortcut { sequence: "Ctrl+E";                  onActivated: app.startExport() }
    Shortcut { sequence: StandardKey.Undo;          onActivated: app.undo() }
    Shortcut { sequence: StandardKey.Redo;          onActivated: app.redo() }
    Shortcut { sequence: "Ctrl+Shift+Z";            onActivated: app.redo() }
    Shortcut { sequence: StandardKey.Delete;        onActivated: if (app.selectedRow >= 0) app.removeSegment(app.selectedRow) }
    Shortcut { sequence: "Space";                   onActivated: if (app.selectedRow >= 0) app.toggleSegment(app.selectedRow, !segmentModel.data(segmentModel.index(app.selectedRow, 0), 257)) }
    Shortcut { sequence: StandardKey.MoveToNextLine;     onActivated: app.selectSegment(Math.min(segmentModel.rowCount() - 1, app.selectedRow + 1)) }
    Shortcut { sequence: StandardKey.MoveToPreviousLine; onActivated: app.selectSegment(Math.max(0, app.selectedRow - 1)) }
    Shortcut { sequence: "I";                       onActivated: app.setIn() }
    Shortcut { sequence: "O";                       onActivated: app.setOut() }
    Shortcut { sequence: ",";                       onActivated: app.previousKeyframe() }
    Shortcut { sequence: ".";                       onActivated: app.nextKeyframe() }

    // ---- Dialogs ----
    Dialog {
        id: aboutDialog
        title: qsTr("About SEDER Video Rewrap")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Close
        contentItem: ColumnLayout {
            spacing: 10
            Label {
                text: qsTr("SEDER Media Suite Video Rewrap")
                font.bold: true
                font.pixelSize: 18
                color: ink
            }
            Label { text: qsTr("Version %1").arg(app.appVersion); color: muted }
            Label {
                text: qsTr("Local-first Qt/Rust desktop tool for keyframe-aligned\nFFmpeg stream-copy exports.")
                color: muted
            }
            Label {
                text: qsTr("Includes FFmpeg as an external dependency (LGPL).\nLicensed under GPL-3.0-only.")
                color: faint
                font.pixelSize: 11
            }
            CheckBox {
                id: updateOptIn
                text: qsTr("Check for updates on launch")
                checked: app.updateChecker.checkOnLaunch
                onToggled: app.updateChecker.checkOnLaunch = checked
            }
            RowLayout {
                CommandButton { text: qsTr("Check Now"); onClicked: app.updateChecker.checkNow() }
                Label { text: app.updateChecker.lastMessage; color: faint; Layout.fillWidth: true; elide: Text.ElideRight }
            }
        }
    }

    Dialog {
        id: settingsDialog
        title: qsTr("FFmpeg Path")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Close
        contentItem: ColumnLayout {
            spacing: 8
            Label {
                text: qsTr("If FFmpeg is not on PATH, point the app at the folder containing ffmpeg, ffprobe, and ffplay.")
                color: muted
                wrapMode: Text.WordWrap
                Layout.maximumWidth: 420
            }
            Label { text: qsTr("Current: %1").arg(app.customFfmpegDir.length ? app.customFfmpegDir : qsTr("(using system PATH)")); color: faint }
            RowLayout {
                CommandButton { text: qsTr("Choose folder..."); onClicked: ffmpegFolderDialog.open() }
                CommandButton { text: qsTr("Use System PATH"); onClicked: app.clearCustomFfmpegDir() }
            }
            Label { text: app.ffmpegVersionText; color: app.ffmpegCompatible ? good : warn; wrapMode: Text.WordWrap; Layout.maximumWidth: 420 }
        }
    }

    FolderDialog {
        id: ffmpegFolderDialog
        title: qsTr("Select FFmpeg folder")
        onAccepted: {
            var p = selectedFolder.toString()
            if (p.startsWith("file://")) p = p.substring(7)
            app.setCustomFfmpegDir(p)
        }
    }

    Dialog {
        id: errorDialog
        title: qsTr("Export Failed")
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Close
        width: Math.min(720, root.width - 80)
        contentItem: ColumnLayout {
            spacing: 8
            Label { text: qsTr("The export did not complete. Details below — share these with support if you need help."); color: muted; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            ScrollView {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                TextArea {
                    readOnly: true
                    wrapMode: TextEdit.Wrap
                    font.family: monoFontFamily
                    font.pixelSize: 11
                    text: app.lastErrorLog
                }
            }
            RowLayout {
                CommandButton { text: qsTr("Copy Log"); onClicked: app.copyLastErrorLogToClipboard() }
                Item { Layout.fillWidth: true }
            }
        }
    }

    Connections {
        target: app
        function onLastErrorLogChanged() {
            if (app.lastErrorLog.length > 0) errorDialog.open()
        }
    }

    // ---- Drag-and-drop ----
    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (drop) => {
            if (drop.hasUrls) {
                for (var i = 0; i < drop.urls.length; ++i) {
                    app.handleDroppedFile(drop.urls[i].toString())
                }
                drop.acceptProposedAction()
            }
        }
    }

    // ---- File menu (recent files) ----
    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            MenuItem { text: qsTr("Open Video...");    onTriggered: app.openSource() }
            MenuItem { text: qsTr("Load Project...");   onTriggered: app.loadProject() }
            MenuItem { text: qsTr("Save Project...");   onTriggered: app.saveProject() }
            MenuSeparator {}
            Menu {
                title: qsTr("Recent Videos")
                enabled: app.recentSources.count > 0
                Repeater {
                    model: app.recentSources
                    delegate: MenuItem {
                        required property string path
                        required property string display
                        text: display.length ? display : path
                        onTriggered: app.openSourcePath(path)
                    }
                }
                MenuSeparator { visible: app.recentSources.count > 0 }
                MenuItem { text: qsTr("Clear"); enabled: app.recentSources.count > 0; onTriggered: app.recentSources.clear() }
            }
            Menu {
                title: qsTr("Recent Projects")
                enabled: app.recentProjects.count > 0
                Repeater {
                    model: app.recentProjects
                    delegate: MenuItem {
                        required property string path
                        required property string display
                        text: display.length ? display : path
                        onTriggered: app.openProjectPath(path)
                    }
                }
                MenuSeparator { visible: app.recentProjects.count > 0 }
                MenuItem { text: qsTr("Clear"); enabled: app.recentProjects.count > 0; onTriggered: app.recentProjects.clear() }
            }
            MenuSeparator {}
            MenuItem { text: qsTr("Export TXT Report..."); onTriggered: app.exportTxtReport() }
            MenuItem { text: qsTr("Export CSV Report..."); onTriggered: app.exportCsvReport() }
            MenuSeparator {}
            MenuItem { text: qsTr("Quit"); onTriggered: Qt.quit() }
        }
        Menu {
            title: qsTr("&Edit")
            MenuItem { text: qsTr("Undo"); enabled: app.canUndo; onTriggered: app.undo() }
            MenuItem { text: qsTr("Redo"); enabled: app.canRedo; onTriggered: app.redo() }
        }
        Menu {
            title: qsTr("&Tools")
            MenuItem { text: qsTr("Recheck FFmpeg"); onTriggered: app.recheckTools() }
            MenuItem { text: qsTr("FFmpeg Path..."); onTriggered: settingsDialog.open() }
            MenuSeparator {}
            MenuItem { text: qsTr("Check for Updates"); onTriggered: app.updateChecker.checkNow() }
        }
        Menu {
            title: qsTr("&Help")
            MenuItem { text: qsTr("About"); onTriggered: aboutDialog.open() }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Update banner ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: app.updateChecker.updateAvailable ? 38 : 0
            visible: app.updateChecker.updateAvailable
            color: warn
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 10
                Label {
                    text: qsTr("Update available: %1").arg(app.updateChecker.latestVersion)
                    color: "white"
                    font.bold: true
                    Layout.fillWidth: true
                }
                Button {
                    text: qsTr("Download")
                    onClicked: app.updateChecker.openUpdateUrl()
                }
                Button {
                    text: qsTr("Dismiss")
                    flat: true
                    onClicked: app.updateChecker.dismiss()
                }
            }
        }

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
                        text: qsTr("SEDER Media Suite Video Rewrap")
                        color: ink
                        font.family: root.uiFontFamily
                        font.pixelSize: 30
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                    MetaLabel { text: qsTr("VOL. 04 / KEYFRAME STREAM COPY") }
                }
                StatusPill {
                    text: app.ffmpegReady && app.ffprobeReady && app.ffmpegCompatible ? qsTr("READY") : qsTr("MISSING")
                    tone: app.ffmpegReady && app.ffprobeReady && app.ffmpegCompatible ? good : bad
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

                    MetaLabel { text: qsTr("01 / TOOLS") }
                    RowLayout {
                        CommandButton { text: qsTr("Recheck FFmpeg"); onClicked: app.recheckTools() }
                        StatusPill {
                            text: app.ffmpegReady && app.ffprobeReady ? qsTr("READY") : qsTr("MISSING")
                            tone: app.ffmpegReady && app.ffprobeReady ? good : bad
                        }
                    }
                    PathLabel { text: app.ffmpegVersionText; Layout.fillWidth: true; wrapMode: Text.Wrap }
                    PathLabel { text: app.logText; Layout.fillWidth: true; wrapMode: Text.Wrap }

                    MetaLabel { text: qsTr("02 / FILES") }
                    CommandButton { text: qsTr("Open Video"); Layout.fillWidth: true; onClicked: app.openSource() }
                    PathLabel { text: app.sourcePath.length ? app.sourcePath : qsTr("No source selected"); Layout.fillWidth: true }
                    CommandButton { text: qsTr("Output File"); Layout.fillWidth: true; onClicked: app.chooseOutput() }
                    PathLabel { text: app.outputPath.length ? app.outputPath : qsTr("No output selected"); Layout.fillWidth: true }

                    MetaLabel { text: qsTr("03 / PROJECT") }
                    RowLayout {
                        CommandButton { text: qsTr("Save"); Layout.fillWidth: true; onClicked: app.saveProject() }
                        CommandButton { text: qsTr("Load"); Layout.fillWidth: true; onClicked: app.loadProject() }
                    }
                    RowLayout {
                        CommandButton { text: qsTr("Undo"); enabled: app.canUndo; Layout.fillWidth: true; onClicked: app.undo() }
                        CommandButton { text: qsTr("Redo"); enabled: app.canRedo; Layout.fillWidth: true; onClicked: app.redo() }
                    }
                    RowLayout {
                        CommandButton { text: qsTr("TXT Report"); Layout.fillWidth: true; onClicked: app.exportTxtReport() }
                        CommandButton { text: qsTr("CSV Report"); Layout.fillWidth: true; onClicked: app.exportCsvReport() }
                    }

                    MetaLabel { text: qsTr("04 / EXPORT") }
                    RowLayout {
                        Layout.fillWidth: true
                        CommandButton { text: qsTr("Concat Single"); Layout.fillWidth: true; onClicked: app.setExportMode("concat_single") }
                        CommandButton { text: qsTr("Separate Files"); Layout.fillWidth: true; onClicked: app.setExportMode("separate_files") }
                    }
                    PathLabel { text: qsTr("Mode: %1").arg(app.exportMode); Layout.fillWidth: true }
                    CommandButton {
                        text: app.busy ? qsTr("Exporting...") : qsTr("Start Export")
                        primary: true
                        enabled: !app.busy
                        Layout.fillWidth: true
                        onClicked: app.startExport()
                    }
                    CommandButton {
                        text: qsTr("Replace File")
                        enabled: !app.busy && app.sourcePath.length > 0
                        Layout.fillWidth: true
                        onClicked: app.replaceFile()
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Renames source to _PREWRAP and exports new file in its place.")
                    }
                    CommandButton {
                        text: qsTr("Cancel Export")
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

                    MetaLabel { text: qsTr("05 / THEME") }
                    RowLayout {
                        CommandButton { text: qsTr("System"); Layout.fillWidth: true; onClicked: app.setTheme("system") }
                        CommandButton { text: qsTr("Light"); Layout.fillWidth: true; onClicked: app.setTheme("light") }
                        CommandButton { text: qsTr("Dark"); Layout.fillWidth: true; onClicked: app.setTheme("dark") }
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
                            MetaLabel { text: qsTr("MEDIA METADATA") }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 4
                                columnSpacing: 10
                                Metric { label: qsTr("Duration"); value: app.durationText; Layout.fillWidth: true }
                                Metric { label: qsTr("Resolution"); value: app.resolutionText; Layout.fillWidth: true }
                                Metric { label: qsTr("Codec"); value: app.codecText; Layout.fillWidth: true }
                                Metric { label: qsTr("Size"); value: app.sizeText; Layout.fillWidth: true }
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
                            MetaLabel { text: qsTr("KEYFRAME NAVIGATION AND SEGMENT CREATION") }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 3
                                columnSpacing: 10
                                Metric { label: qsTr("Keyframes"); value: app.keyframeCount.toString(); Layout.fillWidth: true }
                                Metric { label: qsTr("Current"); value: app.currentKeyframeText; tone: good; Layout.fillWidth: true }
                                Metric { label: qsTr("Selected"); value: app.totalDurationText; Layout.fillWidth: true }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                CommandButton { text: qsTr("Go to Previous Keyframe"); onClicked: app.previousKeyframe(); ToolTip.visible: hovered; ToolTip.text: qsTr("Move playhead to the previous detected keyframe.") }
                                CommandButton { text: qsTr("Go to Next Keyframe"); onClicked: app.nextKeyframe(); ToolTip.visible: hovered; ToolTip.text: qsTr("Move playhead to the next detected keyframe.") }
                                CommandButton { text: qsTr("Preview Selected Keyframe"); onClicked: app.previewCurrent(); ToolTip.visible: hovered; ToolTip.text: qsTr("Play a short preview at the selected keyframe.") }
                                MetaLabel { text: qsTr("JUMP TO TIMECODE"); Layout.leftMargin: 8 }
                                Field {
                                    id: jumpTime
                                    text: "00:00:00.000"
                                    Layout.preferredWidth: 150
                                    Accessible.name: qsTr("Jump to timecode")
                                }
                                CommandButton { text: qsTr("Jump to Timecode"); onClicked: app.jumpToTimecode(jumpTime.text); ToolTip.visible: hovered; ToolTip.text: qsTr("Use HH:MM:SS.mmm format to move to an exact time.") }
                            }
                            MetaLabel { text: qsTr("Tip: Set IN and OUT markers before creating a segment. Shortcuts: I, O, comma/period to step keyframes.") }
                            RowLayout {
                                Layout.fillWidth: true
                                CommandButton { text: qsTr("Set In Marker"); onClicked: app.setIn(); ToolTip.visible: hovered; ToolTip.text: qsTr("Set the start boundary for the segment you are creating.") }
                                StatusPill { text: qsTr("IN %1").arg(app.pendingInText) }
                                CommandButton { text: qsTr("Set Out Marker"); onClicked: app.setOut(); ToolTip.visible: hovered; ToolTip.text: qsTr("Set the end boundary for the segment you are creating.") }
                                StatusPill { text: qsTr("OUT %1").arg(app.pendingOutText) }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                Field {
                                    id: segmentName
                                    placeholderText: qsTr("Segment name")
                                    Layout.preferredWidth: 180
                                    Accessible.name: qsTr("Segment name")
                                }
                                Field {
                                    id: segmentNotes
                                    placeholderText: qsTr("Notes")
                                    Layout.fillWidth: true
                                    Accessible.name: qsTr("Segment notes")
                                }
                                CommandButton {
                                    text: qsTr("Create Segment")
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
                                MetaLabel { text: qsTr("SEGMENT LIST"); Layout.fillWidth: true }
                                StatusPill { text: qsTr("TOTAL %1").arg(app.totalDurationText); tone: good }
                            }
                            Row {
                                Layout.fillWidth: true
                                height: 28
                                Repeater {
                                    model: [qsTr("Enabled"), qsTr("Segment Name"), qsTr("In Marker"), qsTr("Out Marker"), qsTr("Duration"), qsTr("Notes")]
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
                                CommandButton { text: qsTr("Set Enabled"); enabled: app.selectedRow >= 0; onClicked: app.toggleSegment(app.selectedRow, true) }
                                CommandButton { text: qsTr("Set Disabled"); enabled: app.selectedRow >= 0; onClicked: app.toggleSegment(app.selectedRow, false) }
                                CommandButton { text: qsTr("Move Up"); enabled: app.selectedRow > 0; onClicked: app.moveSegmentUp(app.selectedRow) }
                                CommandButton { text: qsTr("Move Down"); enabled: app.selectedRow >= 0 && app.selectedRow + 1 < segmentModel.rowCount(); onClicked: app.moveSegmentDown(app.selectedRow) }
                                CommandButton { text: qsTr("Duplicate"); enabled: app.selectedRow >= 0; onClicked: app.duplicateSegment(app.selectedRow) }
                                CommandButton { text: qsTr("Delete"); enabled: app.selectedRow >= 0; onClicked: app.removeSegment(app.selectedRow) }
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
                    text: app.busy ? qsTr("WORKING") : qsTr("LOG")
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
