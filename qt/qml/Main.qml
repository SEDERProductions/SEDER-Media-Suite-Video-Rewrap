import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtMultimedia

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

    function tableColumnWidth(column) {
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

        return widths[column]
    }
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
    readonly property string uiFontFamily: Qt.application.font.family
    readonly property string monoFontFamily: Qt.platform.os === "osx"
        ? "Menlo"
        : (Qt.platform.os === "windows" ? "Consolas" : "Monospace")

    // Shared denominator for the preview timeline: keyframes are in the app's ms
    // domain, so prefer the probed duration and fall back to the player's own.
    readonly property real previewDurationMs: app.durationMs > 0
        ? app.durationMs
        : (player.duration > 0 ? player.duration : 0)
    // Single-key shortcuts (I/O/Space/arrows) must not fire while typing in a field.
    readonly property bool typing: (jumpTime && jumpTime.activeFocus)
        || (segmentName && segmentName.activeFocus)
        || (segmentNotes && segmentNotes.activeFocus)
    property bool previewingSegment: false
    property string previewError: ""

    function noticeColor(tone) {
        if (tone === "bad")
            return bad
        if (tone === "warn")
            return warn
        if (tone === "good")
            return good
        return accent
    }

    function fmtClock(ms) {
        if (!ms || ms < 0)
            ms = 0
        var totalSeconds = Math.floor(ms / 1000)
        var minutes = Math.floor(totalSeconds / 60)
        var seconds = totalSeconds % 60
        return (minutes < 10 ? "0" : "") + minutes + ":" + (seconds < 10 ? "0" : "") + seconds
    }

    function togglePreviewPlayback() {
        if (player.playbackState === MediaPlayer.PlayingState)
            player.pause()
        else
            player.play()
    }

    color: bg

    component MetaLabel: Label {
        color: faint
        font.family: root.monoFontFamily
        font.styleHint: Font.Monospace
        font.pixelSize: 10
        font.capitalization: Font.AllUppercase
        elide: Text.ElideRight
    }

    component PathLabel: Label {
        color: muted
        font.family: root.monoFontFamily
        font.styleHint: Font.Monospace
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
            font.family: root.monoFontFamily
            font.styleHint: Font.Monospace
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
                Layout.horizontalStretchFactor: 1
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
                Layout.horizontalStretchFactor: 4
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

                    MetaLabel { text: "04 / EXPORT VIDEO" }
                    MetaLabel { text: "Tip: Export requires FFmpeg/FFprobe, a source video, and an output file path." }
                    CommandButton {
                        text: app.busy ? "Exporting..." : "Start Export"
                        primary: true
                        enabled: !app.busy
                        Layout.fillWidth: true
                        onClicked: app.outputExists() ? overwriteDialog.open() : app.startExport()
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
                Layout.horizontalStretchFactor: 9
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
                        id: noticeBanner
                        Layout.fillWidth: true
                        visible: app.noticeText.length > 0
                        implicitHeight: visible ? noticeRow.implicitHeight + 20 : 0
                        readonly property color tone: root.noticeColor(app.noticeTone)
                        color: Qt.rgba(tone.r, tone.g, tone.b, 0.14)
                        border.color: tone
                        RowLayout {
                            id: noticeRow
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10
                            Label {
                                text: app.noticeText
                                color: root.ink
                                font.family: root.uiFontFamily
                                font.pixelSize: 13
                                wrapMode: Text.Wrap
                                Layout.fillWidth: true
                            }
                            CommandButton { text: "Dismiss"; onClicked: app.dismissNotice() }
                        }
                    }

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
                        implicitHeight: compactLayout ? 360 : 420
                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10
                            RowLayout {
                                Layout.fillWidth: true
                                MetaLabel { text: "VIDEO PREVIEW"; Layout.fillWidth: true }
                                StatusPill {
                                    text: app.ffplayReady ? "FFPLAY OK" : "NO FFPLAY"
                                    tone: app.ffplayReady ? good : warn
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                color: "#000000"
                                radius: 4
                                border.color: line
                                clip: true
                                VideoOutput {
                                    id: videoOut
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    fillMode: VideoOutput.PreserveAspectFit
                                }
                                Label {
                                    anchors.centerIn: parent
                                    visible: app.videoUrl.toString().length === 0
                                    text: "Open or drop a video to preview"
                                    color: faint
                                    font.family: root.uiFontFamily
                                    font.pixelSize: 14
                                }
                                Rectangle {
                                    anchors.fill: parent
                                    visible: root.previewError.length > 0
                                    color: Qt.rgba(0, 0, 0, 0.72)
                                    Label {
                                        anchors.centerIn: parent
                                        width: parent.width - 40
                                        text: root.previewError
                                        color: "white"
                                        horizontalAlignment: Text.AlignHCenter
                                        wrapMode: Text.Wrap
                                        font.family: root.uiFontFamily
                                        font.pixelSize: 13
                                    }
                                }
                            }
                            Rectangle {
                                id: timelineTrack
                                Layout.fillWidth: true
                                implicitHeight: 28
                                radius: 4
                                color: panelAlt
                                border.color: line
                                clip: true
                                Rectangle {
                                    visible: app.inMs >= 0 && app.outMs > app.inMs && root.previewDurationMs > 0
                                    color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.25)
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    x: root.previewDurationMs > 0 ? timelineTrack.width * (app.inMs / root.previewDurationMs) : 0
                                    width: root.previewDurationMs > 0 ? timelineTrack.width * ((app.outMs - app.inMs) / root.previewDurationMs) : 0
                                }
                                Repeater {
                                    model: app.keyframesMs
                                    Rectangle {
                                        width: 1
                                        height: 8
                                        y: 4
                                        color: root.faint
                                        x: root.previewDurationMs > 0 ? timelineTrack.width * (modelData / root.previewDurationMs) : 0
                                    }
                                }
                                Rectangle {
                                    width: 2
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    color: root.accent
                                    x: root.previewDurationMs > 0 ? timelineTrack.width * (player.position / root.previewDurationMs) : 0
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    enabled: root.previewDurationMs > 0
                                    onPressed: (mouse) => {
                                        var frac = Math.max(0, Math.min(1, mouse.x / width))
                                        player.setPosition(app.snapToKeyframe(Math.round(frac * root.previewDurationMs)))
                                    }
                                    onPositionChanged: (mouse) => {
                                        if (pressed) {
                                            var frac = Math.max(0, Math.min(1, mouse.x / width))
                                            player.setPosition(app.snapToKeyframe(Math.round(frac * root.previewDurationMs)))
                                        }
                                    }
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                CommandButton {
                                    text: player.playbackState === MediaPlayer.PlayingState ? "Pause" : "Play"
                                    enabled: app.videoUrl.toString().length > 0
                                    onClicked: root.togglePreviewPlayback()
                                }
                                CommandButton {
                                    text: "Preview Segment"
                                    enabled: app.inMs >= 0 && app.outMs > app.inMs
                                    onClicked: {
                                        root.previewingSegment = true
                                        player.setPosition(app.inMs)
                                        player.play()
                                    }
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Play from the IN marker and stop at the OUT marker."
                                }
                                CommandButton {
                                    text: "Seek to Keyframe"
                                    enabled: app.keyframeCount > 0
                                    onClicked: player.setPosition(app.currentKeyframeMs)
                                }
                                Label {
                                    text: root.fmtClock(player.position) + " / " + root.fmtClock(root.previewDurationMs)
                                    color: muted
                                    font.family: root.monoFontFamily
                                    font.pixelSize: 12
                                }
                                Item { Layout.fillWidth: true }
                                CommandButton {
                                    text: "Open in FFplay"
                                    enabled: app.ffplayReady
                                    onClicked: app.previewCurrent()
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Fallback: open the current keyframe in an external FFplay window."
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 12
                                ColumnLayout {
                                    spacing: 4
                                    MetaLabel { text: "IN FRAME " + app.pendingInText }
                                    Rectangle {
                                        Layout.preferredWidth: 132
                                        Layout.preferredHeight: 74
                                        color: panelAlt
                                        border.color: line
                                        radius: 4
                                        clip: true
                                        Image {
                                            anchors.fill: parent
                                            anchors.margins: 1
                                            fillMode: Image.PreserveAspectFit
                                            cache: false
                                            source: app.inThumbUrl
                                            visible: app.inThumbUrl.toString().length > 0
                                        }
                                        MetaLabel {
                                            anchors.centerIn: parent
                                            text: "NO IN FRAME"
                                            visible: app.inThumbUrl.toString().length === 0
                                        }
                                    }
                                }
                                ColumnLayout {
                                    spacing: 4
                                    MetaLabel { text: "OUT FRAME " + app.pendingOutText }
                                    Rectangle {
                                        Layout.preferredWidth: 132
                                        Layout.preferredHeight: 74
                                        color: panelAlt
                                        border.color: line
                                        radius: 4
                                        clip: true
                                        Image {
                                            anchors.fill: parent
                                            anchors.margins: 1
                                            fillMode: Image.PreserveAspectFit
                                            cache: false
                                            source: app.outThumbUrl
                                            visible: app.outThumbUrl.toString().length > 0
                                        }
                                        MetaLabel {
                                            anchors.centerIn: parent
                                            text: "NO OUT FRAME"
                                            visible: app.outThumbUrl.toString().length === 0
                                        }
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
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
                                implicitHeight: 28
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
                                        font.family: column >= 2 ? root.monoFontFamily : root.uiFontFamily
                                        font.styleHint: column >= 2 ? Font.Monospace : Font.SansSerif
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

    MediaPlayer {
        id: player
        source: app.videoUrl
        videoOutput: videoOut
        audioOutput: AudioOutput { id: audioOut }
        onErrorOccurred: function(error, errorString) {
            root.previewError = "In-app preview can't decode this file. Use \"Open in FFplay\".\n" + errorString
        }
        onSourceChanged: {
            root.previewError = ""
            root.previewingSegment = false
        }
        onPositionChanged: {
            if (root.previewingSegment && app.outMs >= 0 && position >= app.outMs) {
                player.pause()
                root.previewingSegment = false
            }
        }
    }

    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: function(drop) {
            if (drop.hasUrls && drop.urls.length > 0)
                app.openDroppedFile(drop.urls[0])
        }
        Rectangle {
            anchors.fill: parent
            visible: parent.containsDrag
            color: Qt.rgba(root.accent.r, root.accent.g, root.accent.b, 0.12)
            border.color: root.accent
            border.width: 2
            radius: 6
            Label {
                anchors.centerIn: parent
                text: "Drop a video file to open"
                color: root.ink
                font.family: root.uiFontFamily
                font.pixelSize: 18
                font.bold: true
            }
        }
    }

    Shortcut { sequence: "Left"; enabled: !root.typing; onActivated: app.previousKeyframe() }
    Shortcut { sequence: "Right"; enabled: !root.typing; onActivated: app.nextKeyframe() }
    Shortcut { sequence: "I"; enabled: !root.typing; onActivated: app.setIn() }
    Shortcut { sequence: "O"; enabled: !root.typing; onActivated: app.setOut() }
    Shortcut { sequence: "Space"; enabled: !root.typing; onActivated: root.togglePreviewPlayback() }

    Dialog {
        id: overwriteDialog
        anchors.centerIn: parent
        modal: true
        title: "Overwrite existing file?"
        standardButtons: Dialog.Yes | Dialog.No
        onAccepted: app.startExport()
        Label {
            text: "The output file already exists and will be overwritten:\n" + app.outputPath
            color: root.ink
            wrapMode: Text.Wrap
            font.family: root.uiFontFamily
            font.pixelSize: 13
        }
    }
}
