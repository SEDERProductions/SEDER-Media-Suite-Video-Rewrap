pragma Singleton
import QtQuick

// Design tokens for the SEDER charcoal workspace. Dark is the design
// target; light mirrors Photoshop's lightest UI brightness. `dark` is
// driven from Main.qml via a Binding to app.darkMode.
QtObject {
    id: theme

    property bool dark: true

    // ---- Surfaces ----
    readonly property color bg: dark ? "#1d1d1d" : "#e4e4e4"
    readonly property color panel: dark ? "#252526" : "#f0f0f0"
    readonly property color panelAlt: dark ? "#2d2d2d" : "#f7f7f7"
    readonly property color headerStrip: dark ? "#2d2d2d" : "#e0e0e0"
    readonly property color inset: dark ? "#161616" : "#ffffff"
    readonly property color overlayScrim: dark ? "#a0000000" : "#60000000"

    // ---- Lines ----
    readonly property color border: dark ? "#3a3a3a" : "#c4c4c4"
    readonly property color borderLight: dark ? "#4a4a4a" : "#b0b0b0"

    // ---- Ink ----
    readonly property color text: dark ? "#d6d6d6" : "#262626"
    readonly property color muted: dark ? "#9e9e9e" : "#6b6b6b"
    readonly property color faint: dark ? "#6e6e6e" : "#8e8e8e"

    // ---- Brand accent (SEDER rust) ----
    readonly property color accent: "#c63b13"
    readonly property color accentHover: "#d94f24"
    readonly property color accentActive: "#a93210"
    readonly property color selectionBg: Qt.rgba(0.776, 0.231, 0.075, 0.30)
    readonly property color textOnAccent: "#ffffff"

    // ---- Semantic ----
    readonly property color good: dark ? "#6fbf73" : "#2e7d32"
    readonly property color warn: dark ? "#d8a657" : "#9a6a00"
    readonly property color bad: dark ? "#e06c5a" : "#b3331d"
    readonly property color info: dark ? "#7fb4d8" : "#2d6f9e"

    // ---- Controls ----
    readonly property color controlBg: dark ? "#383838" : "#fbfbfb"
    readonly property color controlBgHover: dark ? "#434343" : "#ffffff"
    readonly property color controlBgPressed: dark ? "#2a2a2a" : "#e8e8e8"
    readonly property color controlBgDisabled: dark ? "#2a2a2a" : "#ececec"
    readonly property color textDisabled: dark ? "#5c5c5c" : "#a8a8a8"

    // ---- Spacing scale ----
    readonly property int s1: 2
    readonly property int s2: 4
    readonly property int s3: 8
    readonly property int s4: 12
    readonly property int s5: 16
    readonly property int s6: 24

    // ---- Radii ----
    readonly property int radius: 3
    readonly property int radiusLarge: 4

    // ---- Type scale ----
    readonly property int fontSizeTiny: 10
    readonly property int fontSizeSmall: 11
    readonly property int fontSizeBody: 12
    readonly property int fontSizeMedium: 13
    readonly property int fontSizeLarge: 16
    readonly property int fontSizeTitle: 20

    // ---- Motion ----
    readonly property int durFast: 80
    readonly property int durMid: 150
    readonly property int durSlow: 250

    // ---- Fonts ----
    readonly property string uiFont: Qt.application.font.family
    readonly property string monoFont: Qt.platform.os === "osx"
        ? "Menlo"
        : (Qt.platform.os === "windows" ? "Consolas" : "Monospace")
}
