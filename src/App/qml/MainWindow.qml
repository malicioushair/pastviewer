import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Shapes

import PastViewer 1.0

import "ErrorMessageDialog"
import "GuiItems"
import "Helpers"
import "Views"

import "Helpers/colors.js" as Colors

Rectangle {
    id: mainWindowID

    property alias mapAnimationHelper: mapAnimationHelperID
    property bool tipsPromptPending: false

    function openPhotoDetails(photo, thumbnail, title, year) {
        stackViewID.push("Views/PhotoDetails.qml", {
            imageSource: photo,
            thumbnailSource: thumbnail,
            title: title,
            year: year
        })
    }

    function openSettings() {
        stackViewID.push("Views/Settings.qml")
    }

    function showMap() {
        if (stackViewID.depth > 1)
            stackViewID.pop(null, StackView.Immediate)
    }

    function scheduleTipsPrompt() {
        tipsPromptPending = true
        tipsPromptDelayID.restart()
    }

    function tryOpenTipsPrompt() {
        if (!tipsPromptPending || stackViewID.busy)
            return

        tipsPromptPending = false
        const currentItem = stackViewID.currentItem
        if (false
                || !currentItem
                || Qt.application.state !== Qt.ApplicationActive
                || currentItem.objectName === "cameraModePage"
                || currentItem["blocksTipsPrompt"] === true
                || errorDialogID.visible
                || tipsPromptDialogID.visible
                || !guiController.ShouldShowTipsPrompt())
            return

        tipsPromptDialogID.openPrompt()
    }

    color: Colors.palette.bg

    MapAnimationHelper {
        id: mapAnimationHelperID

        map: stackViewID.currentItem ? stackViewID.currentItem.map : null
    }

    StackView {
        id: stackViewID

        anchors.fill: parent
        focus: true

        initialItem: MapPage {
            id: mapPageInstance
        }

        onBusyChanged: {
            if (!busy && mainWindowID.tipsPromptPending)
                tipsPromptDelayID.restart()
        }
    }

    Timer {
        id: tipsPromptDelayID

        interval: 350
        repeat: false
        onTriggered: mainWindowID.tryOpenTipsPrompt()
    }

    ErrorMessageDialog {
        id: errorDialogID

        anchors.centerIn: Overlay.overlay
    }

    TipsPromptDialog {
        id: tipsPromptDialogID

        anchors.centerIn: Overlay.overlay

        onAccepted: guiController.OpenTipsUrl()
        onDiscarded: guiController.DismissTipsPrompt()
    }

    Connections {
        target: guiController

        function onShowErrorDialog(message) {
            errorDialogID.errorMessage = message
            errorDialogID.open()
        }

        function onTipsPromptRequested() {
            mainWindowID.scheduleTipsPrompt()
        }
    }

}
