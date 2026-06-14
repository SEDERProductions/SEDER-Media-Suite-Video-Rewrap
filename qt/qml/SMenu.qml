import QtQuick
import QtQuick.Controls
import Seder.VideoRewrap

Menu {
    id: control

    delegate: SMenuItem { }

    // Qt 6.4 does not reliably size menus to the widest custom delegate;
    // compute it explicitly, padded for shortcut hints.
    width: {
        var widest = 150
        for (var i = 0; i < count; ++i) {
            var item = itemAt(i)
            if (item)
                widest = Math.max(widest, item.implicitWidth
                    + (item.shortcutText !== undefined && item.shortcutText !== "" ? 64 : 0))
        }
        return Math.min(widest + Theme.s4, 420)
    }

    topPadding: Theme.s2
    bottomPadding: Theme.s2

    background: Rectangle {
        implicitWidth: 150
        color: Theme.dark ? "#242424" : "#fafafa"
        border.color: Theme.borderLight
        border.width: 1
        radius: Theme.radiusLarge
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.durFast }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Theme.durFast }
    }
}
