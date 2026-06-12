import QtQuick
import Seder.VideoRewrap

// Shared right-click menu for a segment, used by timeline clips and
// table rows. Set targetRow before popup(). Rename is delegated to the
// host so each surface can use its own editor.
SMenu {
    id: menu

    property int targetRow: -1
    signal renameRequested(int row)

    readonly property bool targetEnabled: {
        var idx = segmentModel.index(targetRow, 0)
        // EnabledRole = Qt::UserRole + 1
        return segmentModel.data(idx, Qt.UserRole + 1) !== false
    }

    SMenuItem {
        text: qsTr("Rename…")
        onTriggered: menu.renameRequested(menu.targetRow)
    }
    SMenuItem {
        text: qsTr("Duplicate")
        onTriggered: app.duplicateSegment(menu.targetRow)
    }
    SMenuItem {
        text: menu.targetEnabled ? qsTr("Disable") : qsTr("Enable")
        shortcutText: qsTr("Space")
        onTriggered: app.toggleSegment(menu.targetRow, !menu.targetEnabled)
    }
    SMenuItem {
        text: qsTr("Seek to Start")
        onTriggered: {
            var idx = segmentModel.index(menu.targetRow, 0)
            // InMsRole = Qt::UserRole + 3
            app.seekToMs(segmentModel.data(idx, Qt.UserRole + 3))
        }
    }
    SMenuSeparator { }
    SMenuItem {
        text: qsTr("Move Earlier")
        enabled: menu.targetRow > 0
        onTriggered: app.moveSegmentUp(menu.targetRow)
    }
    SMenuItem {
        text: qsTr("Move Later")
        enabled: menu.targetRow >= 0 && menu.targetRow + 1 < segmentModel.rowCount()
        onTriggered: app.moveSegmentDown(menu.targetRow)
    }
    SMenuSeparator { }
    SMenuItem {
        text: qsTr("Delete")
        shortcutText: qsTr("Del")
        onTriggered: app.removeSegment(menu.targetRow)
    }
}
