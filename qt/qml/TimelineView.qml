import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import Seder.VideoRewrap

// Zoomable, scrollable timeline: time ruler, keyframe ticks, pending
// IN/OUT range, segment clips, and a keyframe-snapped playhead.
// Coordinate model: x = ms * pxPerMs - contentX.
Item {
    id: timelineView

    readonly property real durationMs: Math.max(1, app.durationMs)
    property real pxPerMs: 0.05
    property real contentX: 0

    readonly property real contentWidth: durationMs * pxPerMs
    readonly property real minPxPerMs: width > 0 ? width / durationMs : 0.0001
    readonly property real maxPxPerMs: 0.5
    readonly property int rulerHeight: 24

    // Zoom as a 0..1 log-scale position between fit and max.
    readonly property real zoomT: maxPxPerMs > minPxPerMs
        ? Math.log(pxPerMs / minPxPerMs) / Math.log(maxPxPerMs / minPxPerMs)
        : 0

    function msToX(ms) { return ms * pxPerMs - contentX }
    function xToMs(x) { return (x + contentX) / pxPerMs }

    function clampContentX(value) {
        return Math.max(0, Math.min(value, Math.max(0, contentWidth - width)))
    }

    // Until the user zooms, the view tracks "fit" through layout changes.
    property bool userZoomed: false

    function zoomAt(anchorX, factor) {
        userZoomed = true
        var msAt = xToMs(anchorX)
        pxPerMs = Math.max(minPxPerMs, Math.min(maxPxPerMs, pxPerMs * factor))
        contentX = clampContentX(msAt * pxPerMs - anchorX)
    }

    function setZoomT(t, anchorX) {
        userZoomed = true
        var target = minPxPerMs * Math.pow(maxPxPerMs / minPxPerMs, Math.max(0, Math.min(1, t)))
        var msAt = xToMs(anchorX === undefined ? width / 2 : anchorX)
        pxPerMs = target
        contentX = clampContentX(msAt * pxPerMs - (anchorX === undefined ? width / 2 : anchorX))
    }

    function fit() {
        userZoomed = false
        // Compute directly: when called from onDurationMsChanged the
        // minPxPerMs binding may not have refreshed yet (handler order).
        pxPerMs = width > 0 ? width / durationMs : 0.0001
        contentX = 0
    }

    function ensurePlayheadVisible() {
        var x = msToX(app.positionMs)
        if (x < 0 || x > width - 2)
            contentX = clampContentX(app.positionMs * pxPerMs - width / 2)
    }

    onWidthChanged: {
        if (width <= 0) return
        if (!userZoomed || pxPerMs < minPxPerMs) fit()
        else contentX = clampContentX(contentX)
    }
    onDurationMsChanged: fit()
    Component.onCompleted: fit()

    onContentXChanged: { ruler.requestPaint(); ticks.requestPaint() }
    onPxPerMsChanged: { ruler.requestPaint(); ticks.requestPaint() }

    Connections {
        target: app
        function onKeyframesChanged() { ticks.requestPaint() }
        function onPositionMsChanged() { timelineView.ensurePlayheadVisible() }
    }

    Connections {
        target: Theme
        function onDarkChanged() { ruler.requestPaint(); ticks.requestPaint() }
    }

    // Ctrl+wheel = anchored zoom, plain wheel = horizontal pan.
    WheelHandler {
        target: null
        onWheel: (event) => {
            if (event.modifiers & Qt.ControlModifier) {
                timelineView.zoomAt(event.x, event.angleDelta.y > 0 ? 1.25 : 0.8)
            } else {
                var delta = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
                timelineView.contentX = timelineView.clampContentX(timelineView.contentX - delta)
            }
        }
    }

    // ---- Time ruler ----
    Canvas {
        id: ruler
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: timelineView.rulerHeight

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = String(Theme.headerStrip)
            ctx.fillRect(0, 0, width, height)

            var steps = [100, 250, 500, 1000, 2000, 5000, 10000, 30000,
                         60000, 300000, 600000, 1800000, 3600000]
            var step = steps[steps.length - 1]
            for (var s = 0; s < steps.length; ++s) {
                if (steps[s] * timelineView.pxPerMs >= 90) { step = steps[s]; break }
            }
            var minor = step / 5

            ctx.strokeStyle = String(Theme.border)
            ctx.fillStyle = String(Theme.muted)
            // Context2D validates families strictly; the generic CSS
            // "monospace" is accepted everywhere, unlike "Monospace".
            ctx.font = "9px monospace"
            ctx.textBaseline = "top"

            var firstMs = Math.floor(timelineView.xToMs(0) / minor) * minor
            var lastMs = timelineView.xToMs(width)

            ctx.beginPath()
            for (var ms = firstMs; ms <= lastMs; ms += minor) {
                if (ms < 0) continue
                var x = Math.round(timelineView.msToX(ms)) + 0.5
                var major = Math.abs(ms % step) < 1
                ctx.moveTo(x, height)
                ctx.lineTo(x, height - (major ? 9 : 4))
                if (major)
                    ctx.fillText(timelineView.formatTick(ms, step), x + 3, 3)
            }
            ctx.stroke()

            ctx.strokeStyle = String(Theme.border)
            ctx.beginPath()
            ctx.moveTo(0, height - 0.5)
            ctx.lineTo(width, height - 0.5)
            ctx.stroke()
        }
    }

    function formatTick(ms, step) {
        var t = Math.max(0, Math.round(ms))
        var milli = t % 1000
        var s = Math.floor(t / 1000) % 60
        var m = Math.floor(t / 60000) % 60
        var h = Math.floor(t / 3600000)
        function pad(n, w) { return String(n).padStart(w, "0") }
        if (step < 1000)
            return pad(m, 2) + ":" + pad(s, 2) + "." + pad(milli, 3)
        if (h > 0)
            return pad(h, 2) + ":" + pad(m, 2) + ":" + pad(s, 2)
        return pad(m, 2) + ":" + pad(s, 2)
    }

    // Scrub from the ruler, Premiere-style press-anywhere.
    MouseArea {
        anchors.fill: ruler
        onPressed: (mouse) => app.seekToMs(timelineView.xToMs(mouse.x))
        onPositionChanged: (mouse) => {
            if (pressed) app.seekToMs(timelineView.xToMs(mouse.x))
        }
    }

    // ---- Track ----
    Rectangle {
        id: trackArea
        anchors.top: ruler.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: scrollBar.top
        color: Theme.inset
        clip: true

        // Empty-track scrubbing (below/between clips).
        MouseArea {
            anchors.fill: parent
            onPressed: (mouse) => app.seekToMs(timelineView.xToMs(mouse.x))
            onPositionChanged: (mouse) => {
                if (pressed) app.seekToMs(timelineView.xToMs(mouse.x))
            }
        }

        Canvas {
            id: ticks
            anchors.fill: parent

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                var kfs = app.keyframesMs
                if (!kfs || kfs.length === 0) return

                // Binary search for the first visible keyframe.
                var startMs = timelineView.xToMs(0)
                var lo = 0, hi = kfs.length - 1, first = kfs.length
                while (lo <= hi) {
                    var mid = (lo + hi) >> 1
                    if (kfs[mid] >= startMs) { first = mid; hi = mid - 1 }
                    else lo = mid + 1
                }

                // Stride so tick density stays below ~1 per 3 px.
                var visibleCount = Math.max(1, (timelineView.xToMs(width) - startMs) * timelineView.pxPerMs / 3)
                var totalVisible = 0
                for (var probe = first; probe < kfs.length && timelineView.msToX(kfs[probe]) <= width; ++probe)
                    ++totalVisible
                var stride = Math.max(1, Math.ceil(totalVisible / Math.max(1, width / 3)))

                ctx.strokeStyle = Qt.rgba(Theme.faint.r, Theme.faint.g, Theme.faint.b, 0.35)
                ctx.beginPath()
                for (var i = first; i < kfs.length; i += stride) {
                    var x = timelineView.msToX(kfs[i])
                    if (x > width) break
                    x = Math.round(x) + 0.5
                    ctx.moveTo(x, 0)
                    ctx.lineTo(x, 5)
                }
                ctx.stroke()
            }
        }

        // ---- Pending IN/OUT range ----
        Rectangle {
            visible: app.hasPendingIn && app.hasPendingOut && app.pendingOutMs > app.pendingInMs
            x: timelineView.msToX(app.pendingInMs)
            width: (app.pendingOutMs - app.pendingInMs) * timelineView.pxPerMs
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.10)
        }

        // IN bracket
        Item {
            visible: app.hasPendingIn
            x: timelineView.msToX(app.pendingInMs)
            width: 9
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            Rectangle { width: 2; height: parent.height; color: Theme.accentHover }
            Rectangle { width: 7; height: 2; color: Theme.accentHover }
            Rectangle { width: 7; height: 2; color: Theme.accentHover; anchors.bottom: parent.bottom }

            MouseArea {
                anchors.fill: parent
                anchors.margins: -3
                cursorShape: Qt.SizeHorCursor
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var ms = timelineView.xToMs(mapToItem(trackArea, mouse.x, 0).x)
                    app.seekToMs(ms)
                    app.setIn()
                }
            }
        }

        // OUT bracket
        Item {
            visible: app.hasPendingOut
            x: timelineView.msToX(app.pendingOutMs) - 7
            width: 9
            anchors.top: parent.top
            anchors.bottom: parent.bottom

            Rectangle { width: 2; height: parent.height; color: Theme.accentHover; anchors.right: parent.right }
            Rectangle { width: 7; height: 2; color: Theme.accentHover; anchors.right: parent.right }
            Rectangle { width: 7; height: 2; color: Theme.accentHover; anchors.right: parent.right; anchors.bottom: parent.bottom }

            MouseArea {
                anchors.fill: parent
                anchors.margins: -3
                cursorShape: Qt.SizeHorCursor
                onPositionChanged: (mouse) => {
                    if (!pressed) return
                    var ms = timelineView.xToMs(mapToItem(trackArea, mouse.x, 0).x)
                    app.seekToMs(ms)
                    app.setOut()
                }
            }
        }

        // ---- Segment clips ----
        Item {
            id: clipLane
            anchors.fill: parent
            anchors.topMargin: 12
            anchors.bottomMargin: 8

            Repeater {
                model: segmentModel
                delegate: TimelineClip {
                    view: timelineView
                    track: trackArea
                    height: clipLane.height
                    onContextMenuRequested: (clipIndex, sceneX, sceneY) => {
                        clipMenu.targetRow = clipIndex
                        clipMenu.popup()
                    }
                }
            }
        }

        // ---- Playhead ----
        Rectangle {
            id: playhead
            x: timelineView.msToX(app.positionMs) - 1
            width: 2
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: Theme.accent
            visible: x >= -2 && x <= trackArea.width
        }
    }

    // Playhead caret on the ruler.
    Shape {
        x: timelineView.msToX(app.positionMs) - 5
        y: timelineView.rulerHeight - 6
        width: 10
        height: 6
        visible: x >= -10 && x <= width
        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            fillColor: Theme.accent
            PathSvg { path: "M0 0h10L5 6z" }
        }
    }

    // ---- Horizontal scroll ----
    SScrollBar {
        id: scrollBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        orientation: Qt.Horizontal
        alwaysVisible: timelineView.contentWidth > timelineView.width + 1
        visible: timelineView.contentWidth > timelineView.width + 1
        height: visible ? 10 : 0
        size: timelineView.contentWidth > 0 ? timelineView.width / timelineView.contentWidth : 1
        position: timelineView.contentWidth > 0 ? timelineView.contentX / timelineView.contentWidth : 0
        onPositionChanged: {
            // Only push back when the user drags the bar.
            if (pressed)
                timelineView.contentX = timelineView.clampContentX(position * timelineView.contentWidth)
        }
    }

    // ---- Clip context menu ----
    SegmentContextMenu {
        id: clipMenu
        onRenameRequested: (row) => {
            renamePopup.row = row
            renamePopup.open()
        }
    }

    // ---- Rename popup ----
    Popup {
        id: renamePopup
        property int row: -1
        x: Math.round((parent.width - width) / 2)
        y: timelineView.rulerHeight + 8
        width: 240
        padding: Theme.s3
        modal: true

        background: Rectangle {
            color: Theme.panel
            border.color: Theme.borderLight
            border.width: 1
            radius: Theme.radiusLarge
        }

        onOpened: {
            var idx = segmentModel.index(row, 1)
            renameField.text = segmentModel.data(idx, Qt.UserRole + 2) || ""
            renameField.selectAll()
            renameField.forceActiveFocus()
        }

        contentItem: SField {
            id: renameField
            placeholderText: qsTr("Segment name")
            Accessible.name: qsTr("Rename segment")
            onAccepted: {
                app.renameSegment(renamePopup.row, text)
                renamePopup.close()
            }
        }
    }
}
