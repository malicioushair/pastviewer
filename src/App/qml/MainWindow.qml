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
    property bool changelogPromptPending: false
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

    function hasPendingPrompt() {
        return changelogPromptPending || tipsPromptPending
    }

    function schedulePromptCheck() {
        promptDelayID.restart()
    }

    function scheduleChangelogPrompt() {
        changelogPromptPending = true
        schedulePromptCheck()
    }

    function scheduleTipsPrompt() {
        tipsPromptPending = true
        schedulePromptCheck()
    }

    function tryOpenPendingPrompt() {
        if (!hasPendingPrompt() || stackViewID.busy)
            return

        const currentItem = stackViewID.currentItem
        if (false
                || !currentItem
                || Qt.application.state !== Qt.ApplicationActive
                || errorDialogID.visible
                || tipsPromptDialogID.visible
                || tipJarDialogID.visible
                || changelogPromptDialogID.visible)
            return

        if (changelogPromptPending) {
            changelogPromptPending = false
            if (GuiController.ShouldShowChangelog())
                changelogPromptDialogID.open()
            else if (tipsPromptPending)
                schedulePromptCheck()
            return
        }

        tipsPromptPending = false
        if (false
                || currentItem.objectName === "cameraModePage"
                || currentItem["blocksTipsPrompt"] === true
                || !GuiController.ShouldShowTipsPrompt())
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
            if (!busy && mainWindowID.hasPendingPrompt())
                mainWindowID.schedulePromptCheck()
        }
    }

    Timer {
        id: promptDelayID

        interval: 350
        repeat: false
        onTriggered: mainWindowID.tryOpenPendingPrompt()
    }

    ErrorMessageDialog {
        id: errorDialogID

        anchors.centerIn: Overlay.overlay

        onClosed: {
            if (mainWindowID.hasPendingPrompt())
                mainWindowID.schedulePromptCheck()
        }
    }

    ChangelogPromptDialog {
        id: changelogPromptDialogID

        anchors.centerIn: Overlay.overlay
        version: GuiController.GetAppVersion()

        onOpened: GuiController.MarkChangelogShown()
        onClosed: {
            if (mainWindowID.hasPendingPrompt())
                mainWindowID.schedulePromptCheck()
        }
    }

    TipsPromptDialog {
        id: tipsPromptDialogID

        anchors.centerIn: Overlay.overlay

        onAccepted: {
            GuiController.RequestTipFlow()
            if (mainWindowID.hasPendingPrompt())
                mainWindowID.schedulePromptCheck()
        }
        onDiscarded: {
            GuiController.DismissTipsPrompt()
            if (mainWindowID.hasPendingPrompt())
                mainWindowID.schedulePromptCheck()
        }
    }

    TipJarDialog {
        id: tipJarDialogID

        anchors.centerIn: Overlay.overlay

        onClosed: {
            if (mainWindowID.hasPendingPrompt())
                mainWindowID.schedulePromptCheck()
        }
    }

    Connections {
        target: GuiController

        function onShowErrorDialog(message) {
            errorDialogID.errorMessage = message
            errorDialogID.open()
        }

        function onTipsPromptRequested() {
            mainWindowID.scheduleTipsPrompt()
        }

        function onTipJarRequested() {
            tipJarDialogID.open()
        }
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.application.state === Qt.ApplicationActive
                    && mainWindowID.hasPendingPrompt())
                mainWindowID.schedulePromptCheck()
        }
    }

    Component.onCompleted: {
        if (GuiController.ShouldShowChangelog())
            mainWindowID.scheduleChangelogPrompt()
    }

}
