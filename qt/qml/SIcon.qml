import QtQuick
import QtQuick.Shapes
import Seder.VideoRewrap

// Themed vector glyph. Glyph data lives in the Icons singleton as 24x24
// path strings; the Scale transform maps the viewBox to the item size.
Item {
    id: icon

    property string name: ""
    property color color: Theme.text

    implicitWidth: 16
    implicitHeight: 16

    Shape {
        width: 24
        height: 24
        transform: Scale {
            xScale: icon.width / 24
            yScale: icon.height / 24
        }
        layer.enabled: true
        layer.samples: 4
        layer.textureSize: Qt.size(Math.max(1, Math.ceil(icon.width)) * 2,
                                   Math.max(1, Math.ceil(icon.height)) * 2)

        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            fillColor: icon.color
            fillRule: ShapePath.OddEvenFill
            PathSvg { path: Icons.paths[icon.name] || "" }
        }
    }
}
