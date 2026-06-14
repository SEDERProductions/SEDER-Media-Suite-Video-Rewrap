import QtQuick
import Seder.VideoRewrap

// Stacks toasts bottom-right. Call show(message, tone).
Item {
    id: host

    function show(message, tone) {
        toastModel.append({ "message": message, "tone": tone || "info" })
        // Keep the stack short; the oldest gives way.
        if (toastModel.count > 4)
            toastModel.remove(0)
    }

    ListModel { id: toastModel }

    Column {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.s5
        spacing: Theme.s3

        Repeater {
            model: toastModel
            delegate: SToast {
                required property int index
                required property var model
                message: model.message
                tone: model.tone
                onDismissed: toastModel.remove(index)
            }
        }
    }
}
