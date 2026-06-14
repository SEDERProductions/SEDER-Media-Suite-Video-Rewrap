import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Seder.VideoRewrap

// Segment list: creation toolbar, virtualized table, row actions.
Item {
    id: panel

    readonly property var columnMinimums: [56, 160, 104, 104, 104, 180]
    readonly property var columnWeights: [0.6, 2.0, 1.2, 1.2, 1.2, 3.0]
    property var cachedColumnWidths: []

    function recalculateColumnWidths() {
        var available = Math.max(620, width - 2)
        var minimumTotal = 0
        var weightTotal = 0
        for (var i = 0; i < columnMinimums.length; ++i) {
            minimumTotal += columnMinimums[i]
            weightTotal += columnWeights[i]
        }
        var extra = Math.max(0, available - minimumTotal)
        var widths = []
        for (var k = 0; k < columnMinimums.length; ++k)
            widths[k] = columnMinimums[k] + extra * (columnWeights[k] / weightTotal)

        // Notes absorbs rounding so the row always fills the panel.
        var notesIndex = columnMinimums.length - 1
        var used = 0
        for (var n = 0; n < notesIndex; ++n)
            used += widths[n]
        widths[notesIndex] = Math.max(columnMinimums[notesIndex], available - used)
        cachedColumnWidths = widths
        table.forceLayout()
    }

    onWidthChanged: recalculateColumnWidths()
    Component.onCompleted: recalculateColumnWidths()

    function columnWidth(column) {
        return cachedColumnWidths[column] || 0
    }

    // Inline editing target; delegates show an editor when they match.
    property int editRow: -1
    property int editColumn: -1

    function commitEdit(row, column, value) {
        if (column === 1)
            app.renameSegment(row, value)
        else if (column === 5)
            app.setSegmentNotes(row, value)
        editRow = -1
        editColumn = -1
    }

    SegmentContextMenu {
        id: rowMenu
        onRenameRequested: (row) => {
            panel.editRow = row
            panel.editColumn = 1
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Creation toolbar ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: Theme.panel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.s3
                anchors.rightMargin: Theme.s3
                spacing: Theme.s3

                SField {
                    id: segmentName
                    placeholderText: qsTr("Segment name")
                    Layout.preferredWidth: 170
                    Accessible.name: qsTr("Segment name")
                }

                SField {
                    id: segmentNotes
                    placeholderText: qsTr("Notes")
                    Layout.fillWidth: true
                    Layout.maximumWidth: 360
                    Accessible.name: qsTr("Segment notes")
                }

                SButton {
                    text: qsTr("Add Segment")
                    iconName: "plus"
                    primary: true
                    tooltip: qsTr("Create a segment from the pending IN/OUT markers.")
                    onClicked: {
                        app.addSegment(segmentName.text, segmentNotes.text)
                        segmentName.text = ""
                        segmentNotes.text = ""
                    }
                }

                Item { Layout.fillWidth: true }

                SPill {
                    text: qsTr("Total %1").arg(app.totalDurationText)
                    tone: Theme.good
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Theme.border
            }
        }

        // ---- Column headers ----
        Row {
            Layout.fillWidth: true
            Layout.preferredHeight: 24

            Repeater {
                model: [qsTr("On"), qsTr("Name"), qsTr("In"), qsTr("Out"), qsTr("Duration"), qsTr("Notes")]
                delegate: Rectangle {
                    required property int index
                    required property string modelData
                    width: panel.columnWidth(index)
                    height: 24
                    color: Theme.headerStrip

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.s3
                        text: modelData
                        color: Theme.faint
                        font.family: Theme.uiFont
                        font.pixelSize: Theme.fontSizeTiny
                        font.bold: true
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 0.8
                    }

                    Rectangle {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 1
                        color: Theme.border
                    }

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        color: Theme.border
                    }
                }
            }
        }

        // ---- Table ----
        TableView {
            id: table
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: segmentModel
            boundsBehavior: Flickable.StopAtBounds
            columnWidthProvider: function (column) { return panel.columnWidth(column) }
            rowHeightProvider: function (row) { return 28 }

            ScrollBar.vertical: SScrollBar { }

            delegate: Rectangle {
                id: cell
                required property int row
                required property int column
                required property string display
                // Declaring any required property turns off context-property
                // injection, so the model object must be required too.
                required property var model
                readonly property bool selected: row === app.selectedRow

                color: selected ? Theme.selectionBg
                    : (row % 2 === 0 ? Theme.panel : Theme.panelAlt)

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 2
                    color: Theme.accent
                    visible: cell.selected && cell.column === 0
                }

                readonly property bool editing: cell.row === panel.editRow
                    && cell.column === panel.editColumn

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.s3
                    anchors.rightMargin: Theme.s3
                    verticalAlignment: Text.AlignVCenter
                    visible: cell.column !== 0 && !cell.editing
                    text: cell.display
                    color: Theme.text
                    font.family: cell.column >= 2 && cell.column <= 4 ? Theme.monoFont : Theme.uiFont
                    font.pixelSize: Theme.fontSizeSmall
                    elide: Text.ElideRight
                    // `model.enabled` dodges the Item.enabled name clash
                    // and stays reactive on toggle.
                    opacity: cell.model.enabled === false ? 0.45 : 1.0
                }

                SCheckBox {
                    visible: cell.column === 0
                    anchors.centerIn: parent
                    checked: cell.model.enabled === true
                    Accessible.name: qsTr("Segment enabled")
                    onToggled: app.toggleSegment(cell.row, checked)
                }

                MouseArea {
                    anchors.fill: parent
                    visible: cell.column !== 0 && !cell.editing
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onPressed: (mouse) => {
                        app.selectSegment(cell.row)
                        if (mouse.button === Qt.RightButton) {
                            rowMenu.targetRow = cell.row
                            rowMenu.popup()
                        }
                    }
                    onDoubleClicked: (mouse) => {
                        if (mouse.button !== Qt.LeftButton) return
                        if (cell.column === 1 || cell.column === 5) {
                            panel.editRow = cell.row
                            panel.editColumn = cell.column
                        } else if (cell.column === 2) {
                            app.seekToMs(cell.model.inMs)
                        } else if (cell.column === 3) {
                            app.seekToMs(cell.model.outMs)
                        }
                    }
                }

                // Inline editor for Name and Notes cells.
                Loader {
                    anchors.fill: parent
                    active: cell.editing
                    sourceComponent: SField {
                        text: cell.display
                        font.pixelSize: Theme.fontSizeSmall
                        Accessible.name: cell.column === 1
                            ? qsTr("Edit segment name") : qsTr("Edit segment notes")
                        Component.onCompleted: {
                            forceActiveFocus()
                            selectAll()
                        }
                        onAccepted: panel.commitEdit(cell.row, cell.column, text)
                        onActiveFocusChanged: {
                            // Focus-out commits, Premiere-style.
                            if (!activeFocus && cell.editing)
                                panel.commitEdit(cell.row, cell.column, text)
                        }
                        Keys.onEscapePressed: {
                            panel.editRow = -1
                            panel.editColumn = -1
                        }
                    }
                }
            }
        }

        // ---- Row actions ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            color: Theme.panel

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: Theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.s3
                anchors.rightMargin: Theme.s3
                spacing: Theme.s1

                SIconButton {
                    iconName: "check"
                    tooltip: qsTr("Toggle segment enabled")
                    shortcutHint: qsTr("Space")
                    enabled: app.selectedRow >= 0
                    onClicked: {
                        var idx = segmentModel.index(app.selectedRow, 0)
                        var enabled = segmentModel.data(idx, Qt.UserRole + 1)
                        app.toggleSegment(app.selectedRow, !enabled)
                    }
                }
                SIconButton {
                    iconName: "arrowUp"
                    tooltip: qsTr("Move segment up")
                    enabled: app.selectedRow > 0
                    onClicked: app.moveSegmentUp(app.selectedRow)
                }
                SIconButton {
                    iconName: "arrowDown"
                    tooltip: qsTr("Move segment down")
                    enabled: app.selectedRow >= 0 && app.selectedRow + 1 < segmentModel.rowCount()
                    onClicked: app.moveSegmentDown(app.selectedRow)
                }
                SIconButton {
                    iconName: "duplicate"
                    tooltip: qsTr("Duplicate segment")
                    enabled: app.selectedRow >= 0
                    onClicked: app.duplicateSegment(app.selectedRow)
                }
                SIconButton {
                    iconName: "trash"
                    tooltip: qsTr("Delete segment")
                    shortcutHint: qsTr("Del")
                    enabled: app.selectedRow >= 0
                    onClicked: app.removeSegment(app.selectedRow)
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: segmentModel.rowCount() === 0
                        ? qsTr("Mark IN and OUT at keyframes, then add a segment")
                        : ""
                    color: Theme.faint
                    font.family: Theme.uiFont
                    font.pixelSize: Theme.fontSizeTiny
                }
            }
        }
    }
}
