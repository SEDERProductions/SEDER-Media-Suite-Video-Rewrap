import QtQuick
import Seder.VideoRewrap

// One segment rendered as a timeline clip with edge retrim handles.
// Lives in a Repeater over segmentModel rows.
Item {
    id: clip

    required property int index
    required property var model
    property var view
    property var track

    signal contextMenuRequested(int clipIndex, real sceneX, real sceneY)

    // Preview values while an edge drag is in flight; committed once on
    // release so the whole trim is a single undo entry.
    property real previewInMs: -1
    property real previewOutMs: -1
    readonly property real effIn: previewInMs >= 0 ? previewInMs : model.inMs
    readonly property real effOut: previewOutMs >= 0 ? previewOutMs : model.outMs
    readonly property bool selected: app.selectedRow === index
    readonly property bool segmentEnabled: model.enabled === true

    x: view.msToX(effIn)
    width: Math.max(3, (effOut - effIn) * view.pxPerMs)
    opacity: segmentEnabled ? 1.0 : 0.38

    Rectangle {
        id: body
        anchors.fill: parent
        radius: Theme.radius
        color: clip.selected
            ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.dark ? 0.55 : 0.45)
            : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, Theme.dark ? 0.28 : 0.22)
        border.color: clip.selected ? Theme.accentHover
            : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.6)
        border.width: clip.selected ? 2 : 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }

        // Premiere-style lighter top edge.
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.leftMargin: 1
            anchors.right: parent.right
            anchors.rightMargin: 1
            height: 1
            color: Qt.rgba(1, 1, 1, 0.12)
        }

        Column {
            anchors.left: parent.left
            anchors.leftMargin: Theme.s3
            anchors.right: parent.right
            anchors.rightMargin: Theme.s3
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1
            visible: clip.width > 40

            Text {
                width: parent.width
                text: clip.model.name
                color: Theme.dark ? "#f3e9e4" : "#fff7f4"
                font.family: Theme.uiFont
                font.pixelSize: Theme.fontSizeSmall
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: clip.model.durationText
                color: Qt.rgba(1, 1, 1, 0.6)
                font.family: Theme.monoFont
                font.pixelSize: Theme.fontSizeTiny
                elide: Text.ElideRight
                visible: clip.height > 34
            }
        }
    }

    Accessible.role: Accessible.ListItem
    Accessible.name: qsTr("Segment %1, %2 to %3").arg(model.name).arg(model.inText).arg(model.outText)

    MouseArea {
        anchors.fill: parent
        anchors.leftMargin: 7
        anchors.rightMargin: 7
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: (mouse) => {
            app.selectSegment(clip.index)
            if (mouse.button === Qt.RightButton) {
                var scene = mapToItem(null, mouse.x, mouse.y)
                clip.contextMenuRequested(clip.index, scene.x, scene.y)
            }
        }
        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton)
                app.seekToMs(clip.model.inMs)
        }
    }

    // Left retrim handle
    MouseArea {
        id: leftHandle
        width: 7
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeHorCursor
        preventStealing: true

        onPressed: clip.previewInMs = clip.model.inMs
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var ms = clip.view.xToMs(mapToItem(clip.track, mouse.x, 0).x)
            var snapped = app.nearestKeyframeMs(ms)
            if (snapped >= 0 && snapped < clip.effOut)
                clip.previewInMs = snapped
        }
        onReleased: {
            var target = clip.previewInMs
            clip.previewInMs = -1
            if (target >= 0 && target !== clip.model.inMs)
                app.setSegmentBounds(clip.index, target, clip.model.outMs)
        }
        onCanceled: clip.previewInMs = -1

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            color: Theme.accentHover
            radius: 1
            visible: leftHandle.containsMouse || leftHandle.pressed
        }
        hoverEnabled: true
    }

    // Right retrim handle
    MouseArea {
        id: rightHandle
        width: 7
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeHorCursor
        preventStealing: true

        onPressed: clip.previewOutMs = clip.model.outMs
        onPositionChanged: (mouse) => {
            if (!pressed) return
            var ms = clip.view.xToMs(mapToItem(clip.track, mouse.x, 0).x)
            var snapped = app.nearestKeyframeMs(ms)
            if (snapped >= 0 && snapped > clip.effIn)
                clip.previewOutMs = snapped
        }
        onReleased: {
            var target = clip.previewOutMs
            clip.previewOutMs = -1
            if (target >= 0 && target !== clip.model.outMs)
                app.setSegmentBounds(clip.index, clip.model.inMs, target)
        }
        onCanceled: clip.previewOutMs = -1

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            color: Theme.accentHover
            radius: 1
            visible: rightHandle.containsMouse || rightHandle.pressed
        }
        hoverEnabled: true
    }
}
