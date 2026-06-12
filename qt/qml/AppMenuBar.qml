import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

MenuBar {
    id: bar

    signal requestFfmpegSettings()
    signal requestAbout()
    signal requestShortcuts()

    delegate: MenuBarItem {
        id: barItem
        implicitHeight: 26
        leftPadding: Theme.s4
        rightPadding: Theme.s4
        contentItem: Text {
            // Strip the mnemonic marker; plain Text has no mnemonic support.
            text: barItem.text.replace("&", "")
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeBody
            color: barItem.highlighted ? Theme.textOnAccent : Theme.text
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: barItem.highlighted ? Theme.accent : "transparent"
            radius: Theme.radius
        }
    }

    background: Rectangle {
        color: Theme.bg
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.border
        }
    }

    SMenu {
        title: qsTr("&File")
        SMenuItem { text: qsTr("Open Video…"); shortcutText: "Ctrl+O"; onTriggered: app.openSource() }
        SMenuItem { text: qsTr("Load Project…"); shortcutText: "Ctrl+Shift+O"; onTriggered: app.loadProject() }
        SMenuItem { text: qsTr("Save Project…"); shortcutText: "Ctrl+S"; onTriggered: app.saveProject() }
        SMenuSeparator { }
        SMenu {
            id: recentVideosMenu
            title: qsTr("Recent Videos")
            enabled: app.recentSources.count > 0
            Instantiator {
                model: app.recentSources
                delegate: SMenuItem {
                    text: model.display.length ? model.display : model.path
                    onTriggered: app.openSourcePath(model.path)
                }
                onObjectAdded: (index, object) => recentVideosMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => recentVideosMenu.removeItem(object)
            }
            SMenuSeparator { }
            SMenuItem { text: qsTr("Clear"); onTriggered: app.recentSources.clear() }
        }
        SMenu {
            id: recentProjectsMenu
            title: qsTr("Recent Projects")
            enabled: app.recentProjects.count > 0
            Instantiator {
                model: app.recentProjects
                delegate: SMenuItem {
                    text: model.display.length ? model.display : model.path
                    onTriggered: app.openProjectPath(model.path)
                }
                onObjectAdded: (index, object) => recentProjectsMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => recentProjectsMenu.removeItem(object)
            }
            SMenuSeparator { }
            SMenuItem { text: qsTr("Clear"); onTriggered: app.recentProjects.clear() }
        }
        SMenuSeparator { }
        SMenuItem { text: qsTr("Export TXT Report…"); onTriggered: app.exportTxtReport() }
        SMenuItem { text: qsTr("Export CSV Report…"); onTriggered: app.exportCsvReport() }
        SMenuSeparator { }
        SMenuItem { text: qsTr("Quit"); onTriggered: Qt.quit() }
    }

    SMenu {
        title: qsTr("&Edit")
        SMenuItem { text: qsTr("Undo"); shortcutText: "Ctrl+Z"; enabled: app.canUndo; onTriggered: app.undo() }
        SMenuItem { text: qsTr("Redo"); shortcutText: "Ctrl+Shift+Z"; enabled: app.canRedo; onTriggered: app.redo() }
    }

    SMenu {
        title: qsTr("&Tools")
        SMenuItem { text: qsTr("Recheck FFmpeg"); onTriggered: app.recheckTools() }
        SMenuItem { text: qsTr("FFmpeg Path…"); onTriggered: bar.requestFfmpegSettings() }
        SMenuSeparator { }
        SMenuItem { text: qsTr("Check for Updates"); onTriggered: app.updateChecker.checkNow() }
    }

    SMenu {
        title: qsTr("&Help")
        SMenuItem { text: qsTr("Keyboard Shortcuts…"); shortcutText: "Ctrl+/"; onTriggered: bar.requestShortcuts() }
        SMenuItem { text: qsTr("About"); onTriggered: bar.requestAbout() }
    }
}
