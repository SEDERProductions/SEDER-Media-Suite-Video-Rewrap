pragma Singleton
import QtQuick

// Solid glyph path data, 24x24 viewBox, rendered by SIcon with an
// even-odd fill rule: enclosed subpaths punch holes, so glyphs must
// avoid overlapping subpaths.
QtObject {
    readonly property var paths: ({
        play: "M8 5l11 7-11 7z",
        pause: "M7 5h4v14H7zM13 5h4v14h-4z",
        skipPrev: "M6 5h2.5v14H6zM18 5v14l-9.5-7z",
        skipNext: "M15.5 5H18v14h-2.5zM6 5l9.5 7L6 19z",
        markIn: "M5 4h5v2.5H7.5v11h2.5V20H5zM11 8.5l6.5 3.5-6.5 3.5z",
        markOut: "M19 4h-5v2.5h2.5v11H14V20h5zM13 8.5L6.5 12l6.5 3.5z",
        plus: "M10.8 5h2.4v5.8H19v2.4h-5.8V19h-2.4v-5.8H5v-2.4h5.8z",
        minus: "M5 10.8h14v2.4H5z",
        trash: "M9.5 3h5l1 1.5H20V7H4V4.5h4.5zM5.5 8.5h13L17.5 21h-11z",
        duplicate: "M9 9h12v12H9zM3 3h12v2.5H5.5V15H3z",
        arrowUp: "M12 4l6.5 6.5h-4.3V20h-4.4v-9.5H5.5z",
        arrowDown: "M12 20l-6.5-6.5h4.3V4h4.4v9.5h4.3z",
        chevronDown: "M12 15.4L5.6 9 7 7.6l5 5 5-5L18.4 9z",
        chevronRight: "M9.4 18.4L8 17l5-5-5-5 1.4-1.4L15.8 12z",
        folder: "M3 5h6.5l2 2.5H21V19H3z",
        save: "M4 4h12.5L20 7.5V20H4zM7 5.5V10h8.5V5.5zM6.5 13v5.5h11V13z",
        film: "M3 4h18v16H3zM6 6h2v2H6zM6 11h2v2H6zM6 16h2v2H6zM16 6h2v2h-2zM16 11h2v2h-2zM16 16h2v2h-2zM10 6.5h4v4.5h-4zM10 13h4v4.5h-4z",
        sliders: "M4 7h3.5v2H4zM12 7h8v2h-8zM8 4.5h3.5v7H8zM4 15h8.5v2H4zM17 15h3v2h-3zM13 12.5h3.5v7H13z",
        undo: "M3.5 7l7-4.5v9zM10.5 5.5H14a7 7 0 0 1 0 14H6v-3h8a4 4 0 0 0 0-8h-3.5z",
        redo: "M20.5 7l-7-4.5v9zM13.5 5.5H10a7 7 0 0 0 0 14h8v-3h-8a4 4 0 0 1 0-8h3.5z",
        zoomIn: "M3.5 10.5a7 7 0 1 0 14 0a7 7 0 1 0 -14 0zM5.7 10.5a4.8 4.8 0 1 0 9.6 0a4.8 4.8 0 1 0 -9.6 0zM9.6 7.5h1.8v2.1h2.1v1.8h-2.1v2.1H9.6v-2.1H7.5V9.6h2.1zM15.8 15.8l4.4 4.4-1.4 1.4-4.4-4.4z",
        zoomOut: "M3.5 10.5a7 7 0 1 0 14 0a7 7 0 1 0 -14 0zM5.7 10.5a4.8 4.8 0 1 0 9.6 0a4.8 4.8 0 1 0 -9.6 0zM7.5 9.6h6v1.8h-6zM15.8 15.8l4.4 4.4-1.4 1.4-4.4-4.4z",
        fit: "M3 3h6v2.2H5.2V9H3zM15 3h6v6h-2.2V5.2H15zM3 15h2.2v3.8H9V21H3zM18.8 15H21v6h-6v-2.2h3.8z",
        check: "M9.2 16.2L4.8 11.8l-1.6 1.6 6 6L21.2 7.4l-1.6-1.6z",
        close: "M5.4 4L12 10.6 18.6 4 20 5.4 13.4 12 20 18.6 18.6 20 12 13.4 5.4 20 4 18.6 10.6 12 4 5.4z",
        alert: "M12 2.5L22.5 20.5H1.5zM10.9 9h2.2v5h-2.2zM10.9 15.8h2.2v2.2h-2.2z",
        info: "M3.5 12a8.5 8.5 0 1 0 17 0a8.5 8.5 0 1 0 -17 0zM10.9 7h2.2v2.2h-2.2zM10.9 10.8h2.2V17h-2.2z",
        edit: "M3 17.2V21h3.8L17.6 10.2l-3.8-3.8zM18.7 9.1l2-2a1.3 1.3 0 0 0 0-1.9l-1.9-1.9a1.3 1.3 0 0 0-1.9 0l-2 2z",
        externalLink: "M5 5h6v2.2H7.2v9.6h9.6V11H19v8H5zM13 3h8v8h-2.2V6.7l-7 7-1.5-1.5 7-7H13z",
        keyboard: "M2 6h20v12H2zM5 9h2v2H5zM9 9h2v2H9zM13 9h2v2h-2zM17 9h2v2h-2zM6 13.5h12v2H6z",
        refresh: "M12 4a8 8 0 0 1 7.4 5h-2.7A5.6 5.6 0 0 0 12 6.4V9L7.5 5.5 12 2zM12 20a8 8 0 0 1-7.4-5h2.7A5.6 5.6 0 0 0 12 17.6V15l4.5 3.5L12 22z",
        download: "M10.8 3h2.4v8h3.3L12 16.5 7.5 11h3.3zM4 18h16v2.2H4z",
        upload: "M12 2.5L16.5 8h-3.3v8h-2.4V8H7.5zM4 18h16v2.2H4z",
        monitor: "M2.5 4h19v13h-19zM4.7 6.2v8.6h14.6V6.2zM10.8 18h2.4v1.5h3.3v1.5H7.5v-1.5h3.3z",
        list: "M4 6h2.2v2.2H4zM8 6h12v2.2H8zM4 10.9h2.2v2.2H4zM8 10.9h12v2.2H8zM4 15.8h2.2V18H4zM8 15.8h12V18H8z",
        swap: "M8 5L3 9l5 4v-3h8V8H8zM16 11l5 4-5 4v-3H8v-2h8z",
        help: "M12 3.5a5.5 5.5 0 0 1 5.5 5.5c0 2.3-1.4 3.4-2.6 4.3-1 .8-1.8 1.4-1.8 2.7h-2.2c0-2.3 1.4-3.4 2.6-4.3 1-.8 1.8-1.4 1.8-2.7a3.3 3.3 0 0 0-6.6 0H6.5A5.5 5.5 0 0 1 12 3.5zM10.9 18.3h2.2v2.2h-2.2z",
        dot: "M8.5 12a3.5 3.5 0 1 0 7 0a3.5 3.5 0 1 0 -7 0z"
    })
}
