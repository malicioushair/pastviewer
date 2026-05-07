import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtLocation
import QtPositioning

ApplicationWindow {
    id: mainWindowID

    property bool isMobile: Qt.platform.os === "android" || Qt.platform.os === "ios"

    width: isMobile ? Screen.width : 1080 / 3
    height: isMobile ? Screen.height : 1920 / 3
    flags: Qt.Window
    visibility: Window.AutomaticVisibility
    visible: true

    title: "Past Viewer"

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