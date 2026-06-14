import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Seder.VideoRewrap

// Dialog base: charcoal body, slim title strip with close button,
// footer row for SButtons. Place body content as children; footer
// buttons via `footerButtons`.
Dialog {
    id: control

    property string titleText: ""
    default property alias bodyContent: bodyColumn.data
    property alias footerButtons: footerRow.data

    modal: true
    padding: 0
    topPadding: 0

    Overlay.modal: Rectangle { color: Theme.overlayScrim }

    background: Rectangle {
        color: Theme.panel
        border.color: Theme.borderLight
        border.width: 1
        radius: Theme.radiusLarge
    }

    header: Rectangle {
        implicitHeight: 32
        color: Theme.headerStrip
        radius: Theme.radiusLarge

        // Square off the bottom corners of the rounded header strip.
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: Theme.radiusLarge
            color: Theme.headerStrip
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: Theme.s4
            anchors.verticalCenter: parent.verticalCenter
            text: control.titleText
            color: Theme.text
            font.family: Theme.uiFont
            font.pixelSize: Theme.fontSizeBody
            font.bold: true
            elide: Text.ElideRight
        }

        SIconButton {
            anchors.right: parent.right
            anchors.rightMargin: Theme.s1
            anchors.verticalCenter: parent.verticalCenter
            iconName: "close"
            iconSize: 11
            tooltip: qsTr("Close")
            onClicked: control.close()
        }

        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.border
        }
    }

    contentItem: ColumnLayout {
        id: bodyColumn
        spacing: Theme.s3
    }

    footer: Item {
        implicitHeight: footerRow.children.length > 0 ? footerRow.implicitHeight + Theme.s4 * 2 : Theme.s3
        Row {
            id: footerRow
            anchors.right: parent.right
            anchors.rightMargin: Theme.s4
            anchors.verticalCenter: parent.verticalCenter
            spacing: Theme.s3
            layoutDirection: Qt.RightToLeft
        }
    }

    // Body margins around user content
    leftPadding: Theme.s5
    rightPadding: Theme.s5
    bottomPadding: Theme.s2

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durMid }
            NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: Theme.durMid; easing.type: Easing.OutCubic }
        }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durFast }
    }
}
