import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtLocation
import QtPositioning

import "Helpers/colors.js" as Colors

ApplicationWindow {
    id: mainWindowID

    property bool isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"

    width: isMobile ? Screen.width : 1080 / 3
    height: isMobile ? Screen.height : 1920 / 3
    flags: isMobile
        ? Qt.Window | Qt.ExpandedClientAreaHint
        : Qt.Window
    visibility: Window.AutomaticVisibility
    visible: true

    color: Colors.palette.bg
    title: "Past Viewer"

    Component.onCompleted: {
        Qt.styleHints.colorScheme = Qt.ColorScheme.Light
    }

    Shortcut {
        sequences: ["Ctrl+R"]
        context: Qt.ApplicationShortcut
        onActivated: {
            if (!guiController.IsDebug())
                return

            guiController.BumpHotReloadToken()
            const base = Qt.resolvedUrl("MainWindow.qml")
            mainWindowLoaderID.source = ""
            mainWindowLoaderID.source = base + "?r=" + Date.now()
        }
    }

    Loader {
        id: mainWindowLoaderID

        anchors.fill: parent

        source: "MainWindow.qml"
        focus: true
    }
}
