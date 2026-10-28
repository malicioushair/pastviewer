import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Shapes

import PastViewer 1.0
import PurchaseController 1.0
import TourController 1.0

import "ErrorMessageDialog"
import "GuiItems"
import "Helpers"
import "Views"

import "Helpers/colors.js" as Colors

Rectangle {
    id: mainWindowID

    property alias mapAnimationHelper: mapAnimationHelperID
    property var editingDraftId: null
    property bool changelogPromptPending: false
    property bool tipsPromptPending: false

    function toMainPage() {
        while (stackViewID.depth != 1) {
            stackViewID.pop()
        }
    }

    function openInspectPhotos(photo) {
        stackViewID.push("Views/InspectPhoto.qml", { imageSource: photo })
    }

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

    function openTours() {
        stackViewID.push("Views/Tours.qml")
    }

    function openAvailableTours() {
        stackViewID.push("Views/AvailableTours.qml")
    }

    function openTourStopInfo(tourStop) {
        stackViewID.push("Views/TourStopInfo.qml", { tourStop: tourStop })
    }

    function openCreateTour() {
        stackViewID.push("Views/CreateTourChoice.qml")
    }

    function openTourDescription(placeOnMap) {
        stackViewID.push("Views/TourDescription.qml", { placeOnMap: placeOnMap })
    }

    function openDraftEditor(id) {
        const record = TourController.GetDraft(id)
        if (record.id !== undefined)
            stackViewID.push("Views/DraftTourEdit.qml", { record: record })
    }

    function openEditDraftMap(id) {
        if (!TourController.SelectDraft(id))
            return
        editingDraftId = id
        GuiController.ChangeMapMode(GuiController.MapMode.PlaceTour)
        showMap()
        if (TourController.tourPath.length > 0)
            mapPageInstance.map.center = TourController.tourPath[0]
    }

    function openDraftStopEditor(id) {
        const record = TourController.GetDraftStop(id)
        if (record.id !== undefined)
            stackViewID.push("Views/DraftTourEdit.qml", { record: record, editingStop: true })
    }

    function openPlaceTourPath() {
        editingDraftId = null
        GuiController.ChangeMapMode(GuiController.MapMode.PlaceTour)
        showMap()
    }

    function finishPlacedTour() {
        GuiController.ChangeMapMode(GuiController.MapMode.Main)
        showMap()
        const returnDraftId = editingDraftId
        editingDraftId = null
        if (returnDraftId !== null) {
            stackViewID.push("Views/DraftTours.qml", StackView.Immediate)
            openDraftEditor(returnDraftId)
        } else {
            openDrafts()
        }
    }

    function openAddTourStopDescription(coordinate) {
        if (coordinate === undefined)
            stackViewID.push("Views/AddTourStopDescription.qml")
        else
            stackViewID.push("Views/AddTourStopDescription.qml", { stopCoordinate: coordinate, placedStop: true })
    }

    function openDrafts() {
        stackViewID.push("Views/DraftTours.qml")
    }

    function openTourInfoView(tour) {
        stackViewID.push("Views/TourInfo.qml", {tour: tour})
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

    // Qt 6.10 delivers Android system Back as a window close request.
    function handleSystemBack() {
        if (stackViewID.depth <= 1)
            return false
        if (!stackViewID.busy)
            stackViewID.pop()
        return true
    }

    // Keep the stack unchanged until the interactive back transition completes.
    Item {
        id: backGestureAreaID

        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 24
        z: 2
        enabled: Qt.platform.os === "ios" && stackViewID.depth > 1 && !stackViewID.busy

        property var outgoing: null
        property var incoming: null
        property var savedIncoming: null
        property real outgoingX: 0
        property real progress: 0
        property bool completing: false
        readonly property bool tracking: outgoing !== null

        function begin() {
            outgoing = stackViewID.currentItem
            incoming = stackViewID.get(stackViewID.depth - 2)
            outgoingX = outgoing.x
            savedIncoming = { x: incoming.x, opacity: incoming.opacity, visible: incoming.visible }
            incoming.visible = true
            incoming.opacity = 1
            updatePages()
        }

        function updatePages() {
            if (!tracking)
                return
            outgoing.x = outgoingX + progress * stackViewID.width
            incoming.x = -stackViewID.width * 0.25 * (1 - progress)
        }

        function restore() {
            settleID.stop()
            if (!tracking)
                return
            outgoing.x = outgoingX
            incoming.x = savedIncoming.x
            incoming.opacity = savedIncoming.opacity
            incoming.visible = savedIncoming.visible
            outgoing = null
            incoming = null
            savedIncoming = null
            progress = 0
        }

        function finish(commit) {
            if (!tracking || settleID.running)
                return
            completing = commit
            settleID.to = commit ? 1 : 0
            settleID.start()
        }

        onProgressChanged: updatePages()
        onEnabledChanged: { if (!enabled) restore() }
        onWidthChanged: restore()

        NumberAnimation {
            id: settleID
            target: backGestureAreaID
            property: "progress"
            duration: 220
            easing.type: Easing.OutCubic
            onFinished: {
                const commit = backGestureAreaID.completing
                        && stackViewID.currentItem === backGestureAreaID.outgoing
                backGestureAreaID.restore()
                if (commit)
                    stackViewID.pop(StackView.Immediate)
            }
        }

        Connections {
            target: stackViewID
            function onCurrentItemChanged() { backGestureAreaID.restore() }
            function onWidthChanged() { backGestureAreaID.restore() }
        }

        Connections {
            target: Qt.application
            function onStateChanged() {
                if (Qt.application.state !== Qt.ApplicationActive)
                    backGestureAreaID.restore()
            }
        }

        DragHandler {
            id: backDragID
            target: null
            enabled: !settleID.running
            maximumPointCount: 1
            yAxis.enabled: false

            onActiveChanged: {
                if (active)
                    backGestureAreaID.begin()
            }
            onTranslationChanged: {
                if (active && backGestureAreaID.tracking) {
                    const dx = centroid.position.x - centroid.pressPosition.x
                    backGestureAreaID.progress = Math.max(0, Math.min(1, dx / stackViewID.width))
                }
            }
            onGrabChanged: (transition, point) => {
                if (transition === PointerDevice.CancelGrabExclusive) {
                    backGestureAreaID.finish(false)
                } else if (transition === PointerDevice.UngrabExclusive && backGestureAreaID.tracking) {
                    const dx = point.position.x - point.pressPosition.x
                    const dy = point.position.y - point.pressPosition.y
                    const forwardFlick = point.velocity.x > 700 && dx > 30
                    backGestureAreaID.finish(dx > Math.abs(dy) * 1.5
                                            && (backGestureAreaID.progress > 0.4 || forwardFlick))
                }
            }
        }
    }

    // Prevent page controls from activating while a swipe is settling.
    MouseArea {
        anchors.fill: parent
        z: 1
        enabled: backGestureAreaID.tracking
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
            PurchaseController.RequestTipFlow()
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
    }

    Connections {
        target: PurchaseController

        function onShowErrorDialog(message) {
            errorDialogID.errorMessage = message
            errorDialogID.open()
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
