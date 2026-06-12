import QtQuick
import Seder.VideoRewrap

// Monospace timecode readout, the signature element of any NLE.
Text {
    property bool emphasized: false

    font.family: Theme.monoFont
    font.pixelSize: emphasized ? Theme.fontSizeLarge : Theme.fontSizeBody
    font.letterSpacing: 0.5
    color: emphasized ? Theme.accentHover : Theme.muted
    verticalAlignment: Text.AlignVCenter

    Accessible.role: Accessible.StaticText
    Accessible.name: text
}
