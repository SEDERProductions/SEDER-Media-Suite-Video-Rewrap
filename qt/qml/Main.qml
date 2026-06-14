import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Seder.VideoRewrap

ApplicationWindow {
    id: root

    width: Math.max(minimumWidth, Math.min(1920, Screen.width * 0.86))
    height: Math.max(minimumHeight, Math.min(1280, Screen.height * 0.9))
    minimumWidth: 980
    minimumHeight: 640
    visible: true
    title: qsTr("SEDER Media Suite Video Rewrap")
    color: Theme.bg

    // Keep the Fusion fallbacks (dialog button boxes, combo popups,
    // native-ish bits) on the charcoal palette.
    palette.window: Theme.bg
    palette.windowText: Theme.text
    palette.base: Theme.inset
    palette.text: Theme.text
    palette.button: Theme.controlBg
    palette.buttonText: Theme.text
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.textOnAccent
    palette.mid: Theme.border
    palette.dark: Theme.border
    palette.light: Theme.borderLight
    palette.placeholderText: Theme.faint

    // The Theme singleton cannot see context properties; drive it here.
    Binding {
        target: Theme
        property: "dark"
        value: app.darkMode
    }

    Component.onCompleted: {
        var w = uiSettings.value("windowWidth", 0)
        var h = uiSettings.value("windowHeight", 0)
        if (w > 0 && h > 0) {
            width = Math.max(minimumWidth, Math.min(Screen.width, w))
            height = Math.max(minimumHeight, Math.min(Screen.height, h))
        }
        var upper = uiSettings.value("upperSplitState")
        if (upper)
            upperSplit.restoreState(upper)
        var main = uiSettings.value("mainSplitState")
        if (main)
            mainSplit.restoreState(main)
        leftTabs.currentIndex = uiSettings.value("leftTabIndex", 0)
        bottomTabs.currentIndex = uiSettings.value("bottomTabIndex", 0)
    }

    onClosing: {
        uiSettings.setValue("windowWidth", width)
        uiSettings.setValue("windowHeight", height)
        uiSettings.setValue("upperSplitState", upperSplit.saveState())
        uiSettings.setValue("mainSplitState", mainSplit.saveState())
        uiSettings.setValue("leftTabIndex", leftTabs.currentIndex)
        uiSettings.setValue("bottomTabIndex", bottomTabs.currentIndex)
    }

    // True while a text input owns focus. Single-letter and bare-Delete
    // shortcuts must defer to typing — without this guard, typing into
    // Notes would delete the selected segment.
    function isTextInputFocused() {
        var item = root.activeFocusItem
        if (!item) return false
        var name = item.toString()
        return name.indexOf("QQuickTextField") >= 0
            || name.indexOf("QQuickTextInput") >= 0
            || name.indexOf("QQuickTextEdit") >= 0
            || name.indexOf("QQuickTextArea") >= 0
    }

    // ---- Global keyboard shortcuts ----
    Shortcut { sequence: StandardKey.Open;          onActivated: app.openSource() }
    Shortcut { sequence: "Ctrl+Shift+O";            onActivated: app.loadProject() }
    Shortcut { sequence: StandardKey.Save;          onActivated: app.saveProject() }
    Shortcut { sequence: "Ctrl+E";                  onActivated: app.startExport() }
    Shortcut { sequences: [StandardKey.Undo];       onActivated: app.undo() }
    Shortcut { sequence: StandardKey.Redo;          onActivated: app.redo() }
    Shortcut { sequence: "Ctrl+Shift+Z";            onActivated: app.redo() }
    Shortcut { sequence: "Ctrl+/";                  onActivated: shortcutsDialog.open() }
    Shortcut {
        sequences: [StandardKey.Delete]
        enabled: !root.isTextInputFocused()
        onActivated: if (app.selectedRow >= 0) app.removeSegment(app.selectedRow)
    }
    Shortcut {
        sequence: "Space"
        enabled: !root.isTextInputFocused()
        onActivated: {
            if (app.selectedRow < 0) return
            var idx = segmentModel.index(app.selectedRow, 0)
            // EnabledRole = Qt::UserRole + 1
            var currentlyEnabled = segmentModel.data(idx, Qt.UserRole + 1)
            app.toggleSegment(app.selectedRow, !currentlyEnabled)
        }
    }
    Shortcut {
        sequence: StandardKey.MoveToNextLine
        enabled: !root.isTextInputFocused()
        onActivated: app.selectSegment(Math.min(segmentModel.rowCount() - 1, app.selectedRow + 1))
    }
    Shortcut {
        sequence: StandardKey.MoveToPreviousLine
        enabled: !root.isTextInputFocused()
        onActivated: app.selectSegment(Math.max(0, app.selectedRow - 1))
    }
    Shortcut {
        sequence: "I"
        enabled: !root.isTextInputFocused()
        onActivated: app.setIn()
    }
    Shortcut {
        sequence: "O"
        enabled: !root.isTextInputFocused()
        onActivated: app.setOut()
    }
    Shortcut {
        sequence: ","
        enabled: !root.isTextInputFocused()
        onActivated: app.previousKeyframe()
    }
    Shortcut {
        sequence: "."
        enabled: !root.isTextInputFocused()
        onActivated: app.nextKeyframe()
    }
    Shortcut {
        sequence: "Home"
        enabled: !root.isTextInputFocused()
        onActivated: app.seekToMs(0)
    }
    Shortcut {
        sequence: "End"
        enabled: !root.isTextInputFocused()
        onActivated: app.seekToMs(app.durationMs)
    }
    Shortcut {
        sequence: "+"
        enabled: !root.isTextInputFocused()
        onActivated: timelinePanel.zoomIn()
    }
    Shortcut {
        sequence: "-"
        enabled: !root.isTextInputFocused()
        onActivated: timelinePanel.zoomOut()
    }

    // ---- Menu bar ----
    menuBar: AppMenuBar {
        onRequestFfmpegSettings: ffmpegSettingsDialog.open()
        onRequestAbout: aboutDialog.open()
        onRequestShortcuts: shortcutsDialog.open()
    }

    header: Column {
        HeaderBar { width: parent.width }
        UpdateBanner { width: parent.width }
    }

    footer: StatusBar { }

    // ---- Workspace ----
    WelcomeView {
        anchors.fill: parent
        visible: app.sourcePath.length === 0
    }

    SplitView {
        id: mainSplit
        anchors.fill: parent
        visible: app.sourcePath.length > 0
        orientation: Qt.Vertical

        handle: Rectangle {
            implicitWidth: 5
            implicitHeight: 5
            color: Theme.bg
            Rectangle {
                anchors.centerIn: parent
                width: parent.width
                height: 1
                color: SplitHandle.pressed ? Theme.accent
                    : SplitHandle.hovered ? Theme.borderLight : Theme.border
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
            }
        }

        SplitView {
            id: upperSplit
            SplitView.fillHeight: true
            SplitView.minimumHeight: 260
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 5
                implicitHeight: 5
                color: Theme.bg
                Rectangle {
                    anchors.centerIn: parent
                    width: 1
                    height: parent.height
                    color: SplitHandle.pressed ? Theme.accent
                        : SplitHandle.hovered ? Theme.borderLight : Theme.border
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                }
            }

            SPanelTabs {
                id: leftTabs
                SplitView.preferredWidth: 320
                SplitView.minimumWidth: 260
                SplitView.maximumWidth: 480
                titles: [qsTr("Project"), qsTr("Export")]

                ProjectPanel { }
                ExportPanel { }
            }

            MonitorPanel {
                SplitView.fillWidth: true
                SplitView.minimumWidth: 420
            }
        }

        SPanelTabs {
            id: bottomTabs
            SplitView.preferredHeight: 250
            SplitView.minimumHeight: 170
            titles: [qsTr("Timeline"), qsTr("Segments")]

            TimelinePanel { id: timelinePanel }
            SegmentsPanel { }
        }
    }

    // ---- Drag-and-drop ----
    DropArea {
        id: dropArea
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

    Rectangle {
        anchors.fill: parent
        visible: dropArea.containsDrag
        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.08)
        border.color: Theme.accent
        border.width: 2
        z: 100

        Rectangle {
            anchors.centerIn: parent
            width: dropHint.implicitWidth + Theme.s6 * 2
            height: 48
            radius: Theme.radiusLarge
            color: Theme.dark ? "#0d0d0d" : "#ffffff"
            border.color: Theme.accent
            border.width: 1

            Text {
                id: dropHint
                anchors.centerIn: parent
                text: qsTr("Drop video or project file")
                color: Theme.text
                font.family: Theme.uiFont
                font.pixelSize: Theme.fontSizeMedium
                font.bold: true
            }
        }
    }

    // ---- Dialogs ----
    AboutDialog {
        id: aboutDialog
        anchors.centerIn: parent
    }

    FfmpegSettingsDialog {
        id: ffmpegSettingsDialog
        anchors.centerIn: parent
    }

    ErrorLogDialog {
        id: errorDialog
        anchors.centerIn: parent
        width: Math.min(680, root.width - 80)
    }

    ShortcutsDialog {
        id: shortcutsDialog
        anchors.centerIn: parent
    }

    Connections {
        target: app
        function onLastErrorLogChanged() {
            if (app.lastErrorLog.length > 0) errorDialog.open()
        }
        function onToastRequested(message, tone) {
            toastHost.show(message, tone)
        }
    }

    ToastHost {
        id: toastHost
        anchors.fill: parent
        z: 90
    }
}
