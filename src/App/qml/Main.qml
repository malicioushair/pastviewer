import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

ApplicationWindow {
    id: mainWindowID

    // on a mobile device, the size is adjusted to the device screen
    // these fields just represent how it is approximately displayed on a phone
    width: 1080 / 3
    height: 1920 / 3
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