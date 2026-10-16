import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Shapes

import PastViewer 1.0
import TourController 1.0

import "../GuiItems"
import "../Helpers"

import "../Helpers/colors.js" as Colors
import "../Helpers/utils.js" as Utils

Item {
    id: mapPageID

    readonly property var positionSource: PastVuModelController.GetPositionSource()
    property alias map: mapViewID.internalMap

    property int mode: GuiController.MapMode.Main
    property var tourPathItem: null

    function updateOnboarding() {
        mapOnboardingID.active = mode === GuiController.MapMode.Main
            && !GuiController.IsOnboardingStepCompleted(mapOnboardingID.completionKey)
        displayTourOnboardingID.active = mode === GuiController.MapMode.DisplayTour
            && !GuiController.IsOnboardingStepCompleted(displayTourOnboardingID.completionKey)
    }

    function showTourPath() {
        if (tourPathItem) {
            mapID.removeMapItem(tourPathItem)
            tourPathItem.destroy()
            tourPathItem = null
        }
        tourPathItem = tourPathComponentID.createObject(mapID)
        mapID.addMapItem(tourPathItem)
        mapID.follow = false
        const raw = TourController.currentTour?.tourPath
        if (raw && raw.length > 0) {
            const helper = mainWindowID.mapAnimationHelper
            const start = QtPositioning.coordinate(raw[0].latitude, raw[0].longitude)
            const here = mapPageID.positionSource.coordinate
            const mid = QtPositioning.coordinate(
                (start.latitude + here.latitude) / 2,
                (start.longitude + here.longitude) / 2
            )
            function flyToStart() {
                helper.animateMapCenterAndZoom(start, 17, start, 1500)
            }
            function onStep1Finished() {
                helper.animationFinished.disconnect(onStep1Finished)
                flyToStart()
            }
            helper.animationFinished.connect(onStep1Finished)
            helper.animateMapCenterAndZoom(mid, 15, start, 3000)
        }
    }

    function hideTourPath() {
        if (!tourPathItem)
            return

        mapID.removeMapItem(tourPathItem)
        tourPathItem.destroy()
        tourPathItem = null
    }

    onModeChanged: {
        if (mode === GuiController.MapMode.PlaceTour) {
            TourController.StopRecording()
            mapID.follow = false
        }

        if (mode === GuiController.MapMode.DisplayTour)
            showTourPath()
        else
            hideTourPath()

        displayTourOnboardingID.currentIndex = 0
        updateOnboarding()
    }

    Connections {
        target: GuiController
        function onOnboardingReset() {
            mapOnboardingID.currentIndex = 0
            displayTourOnboardingID.currentIndex = 0
            mapPageID.updateOnboarding()
        }
        function onMapModeChanged(mode) {
            mapPageID.mode = mode
        }
    }

    Connections {
        target: TourController
        function onSelectedStopChanged(stop) {
            mapAnimationHelperID.animateMapCenter(stop.coords)
        }
    }

    // Stack-level onboarding for the main map view
    OnboardingOverlay {
        id: mapOnboardingID

        anchors.fill: parent

        z: 1000

        completionKey: "MapPageIntro"
        active: mapPageID.mode === GuiController.MapMode.Main && !GuiController.IsOnboardingStepCompleted(completionKey)
        centered: currentIndex === 4 || currentIndex === 2
        topped: currentIndex === 5 || currentIndex === 6

        highlightSteps: [
            { target: toursButtonID, stepIndex: 2 },
            { target: recenterID, stepIndex: 3 },
            { target: subAreaLoaderID.item, stepIndex: 4 },
            { target: compassID, stepIndex: 5 },
            { target: settingsButtonID, stepIndex: 6 },
        ]

        steps: [
            {
                "title": qsTr("Explore history around you"),
                "body": qsTr("Each marker on the map is a historical photo. Move the map or walk around to discover what was here in the past.")
            },
            {
                "title": qsTr("Use the map like any other"),
                "body": qsTr("Drag with one finger to pan, pinch to zoom and rotate.")
            },
            {
                "title": qsTr("Explore with a tour"),
                "body": qsTr("Tap the Tours button below Settings to browse available tours. Choose a tour to read its description and start exploring its route and stops.")
            },
            {
                "title": qsTr("Jump back to your position"),
                "body": qsTr("If you move the map away, tap the recenter button in the lower-right corner to follow your current location again.")
            },
            {
                "title": qsTr("History near you"),
                "body": qsTr("Swipe the History near you row at the bottom. Tap a card to move the map, double-tap to open the photo.")
            },
            {
                "title": qsTr("Reset North"),
                "body": qsTr("In case you lost your direction, tap the compass to snap the map back to north.")
            },
            {
                "title": qsTr("Tune what you see"),
                "body": qsTr("Open Settings in the top-left to change language, adjust the time range, or focus only on the nearest places.")
            }
        ]
    }

    OnboardingOverlay {
        id: displayTourOnboardingID

        anchors.fill: parent
        z: 1000

        completionKey: "DisplayTourIntro"
        active: mapPageID.mode === GuiController.MapMode.DisplayTour
            && !GuiController.IsOnboardingStepCompleted(completionKey)
        centered: true

        highlightSteps: mapPageID.mode === GuiController.MapMode.DisplayTour && subAreaLoaderID.item
            ? subAreaLoaderID.item.tourOnboardingHighlights : []

        steps: [
            {
                "title": qsTr("Follow the tour"),
                "body": qsTr("The map shows the tour route and its numbered stops. Follow the route to explore each place.")
            },
            {
                "title": qsTr("Your selected stop"),
                "body": qsTr("This panel shows the name of the selected stop. The map highlights its number on the route.")
            },
            {
                "title": qsTr("Browse the stops"),
                "body": qsTr("Use the left and right arrows to select the previous or next stop. The map moves to the selected stop.")
            },
            {
                "title": qsTr("Explore a stop"),
                "body": qsTr("When you reach the selected stop, tap I'm here to open its details.")
            },
            {
                "title": qsTr("Leave the tour"),
                "body": qsTr("Tap Exit Tour to return to the regular map and nearby historical photos.")
            }
        ]
    }

    ColumnLayout {
        id: columnLayoutID

        anchors.fill: parent

        Rectangle {
            id: noLocationWarningID

            Layout.alignment: Qt.AlignHCenter
            Layout.margins: 8

            Layout.preferredWidth: warningTextID.implicitWidth + 64
            Layout.preferredHeight: warningTextID.implicitHeight + 16

            visible: !positionSource.positionAvailable

            radius: 8
            color: Colors.palette.bg
            border.color: Colors.palette.accent
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: "⚠"
                    font.pixelSize: 16
                    color: Colors.palette.accent
                }

                Text {
                    id: warningTextID

                    text: qsTr("Current position is not available")
                    color: Colors.palette.text
                    font.pixelSize: 14
                }

                Text {
                    text: "⚠"
                    font.pixelSize: 16
                    color: Colors.palette.accent
                }
            }
        }

        MapView {
            id: mapViewID

            property alias internalMap: mapID

            Layout.fillWidth: true
            Layout.fillHeight: true

            Compass {
                id: compassID

                anchors {
                    top: parent.top
                    left: parent.left
                    margins: 12
                }

                width: 48
                height: 48
                z: 999   // above the map
            }

            SettingsButton {
                id: settingsButtonID

                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 12
                }

                z: 999   // above the map

                onClicked: mainWindowID.openSettings()
            }

            ToursButton {
                id: toursButtonID

                anchors {
                    top: settingsButtonID.bottom
                    right: parent.right
                    margins: 12
                }

                z: 999   // above the map

                onClicked: mainWindowID.openAvailableTours()
            }

            Map {
                id: mapID

                property geoCoordinate startCentroid
                property bool follow: true

                function updateViewCoordinates() {
                    if (mapID.width <= 0 || mapID.height <= 0)
                        return

                    const topLeftCoord = mapID.toCoordinate(Qt.point(0, 0), false)
                    const bottomRightCoord = mapID.toCoordinate(Qt.point(mapID.width, mapID.height), false)
                    PastVuModelController.SetViewportCoordinates(QtPositioning.rectangle(topLeftCoord, bottomRightCoord))
                }

                anchors.fill: parent

                copyrightsVisible: false
                onBearingChanged: updateViewCoordinates()
                // Mode-specific panels change the visible map area.
                onWidthChanged: Qt.callLater(updateViewCoordinates)
                onHeightChanged: Qt.callLater(updateViewCoordinates)

                plugin: Plugin {
                    id: mapPluginID

                    name: "osm"

                    PluginParameter {
                        name: "osm.mapping.custom.host"
                        value: "https://tiles.stadiamaps.com/tiles/outdoors/%z/%x/%y.png?api_key=" + PastVuModelController.GetMapHostApiKey()
                    }
                }

                activeMapType: supportedMapTypes.find((map) => { return map.style === MapType.CustomMap })

                onZoomLevelChanged: (zoomLevel) => { PastVuModelController.zoomLevel = zoomLevel }
                Component.onCompleted: {
                    zoomLevel = PastVuModelController.zoomLevel
                    Utils.setTimeout(updateViewCoordinates, 300) // delaying the call to update to allow some time for the map to load (otherwise we'll get a bad reply)
                }

                Connections {
                    target: mainWindowID.mapAnimationHelper
                    function onAnimatedLatChanged() {
                        if (!mainWindowID.mapAnimationHelper.running)
                            return
                        mapID.center = QtPositioning.coordinate(mainWindowID.mapAnimationHelper.animatedLat, mainWindowID.mapAnimationHelper.animatedLon)
                    }
                    function onAnimatedLonChanged() {
                        if (!mainWindowID.mapAnimationHelper.running)
                            return

                        mapID.center = QtPositioning.coordinate(mainWindowID.mapAnimationHelper.animatedLat, mainWindowID.mapAnimationHelper.animatedLon)
                    }
                    function onAnimatedZoomChanged() {
                        mapID.zoomLevel = mainWindowID.mapAnimationHelper.animatedZoom
                    }
                }

                Binding {
                    id: followCenterID

                    target: mapID
                    property: "center"
                    value: positionSource.coordinate
                    when: mapID.follow && positionSource.coordinate.isValid
                    restoreMode: Binding.RestoreNone
                }

                Component {
                    id: tourPathComponentID
                    TourPath {}
                }

                MapPolyline {
                    path: TourController.tourPath
                    line.width: 4
                    line.color: Colors.palette.accent
                    autoFadeIn: false
                    visible: mapPageID.mode === GuiController.MapMode.PlaceTour && path.length >= 2
                }

                MapItemView {
                    id: placedStopMarkersID

                    z: 1
                    model: TourController.placedStopMarkers
                    visible: mapPageID.mode === GuiController.MapMode.PlaceTour
                    delegate: MapQuickItem {
                        id: stopMarkerID

                        readonly property int itemSize: 20
                        readonly property int stopNumber: modelData.number

                        z: 1
                        coordinate: QtPositioning.coordinate(modelData.latitude, modelData.longitude)
                        anchorPoint: Qt.point(stopMarkerID.itemSize / 2, stopMarkerID.itemSize / 2)
                        sourceItem: Rectangle {
                            implicitWidth: 20
                            implicitHeight: width
                            radius: width
                            color: "white"
                            border {
                                color: Colors.palette.accentAlt
                                width: 1
                            }
                            Label {
                                anchors.centerIn: parent
                                text: stopMarkerID.stopNumber
                            }
                        }
                    }
                }

                Item {
                    id: centerMarkerID

                    anchors.centerIn: parent
                    width: 28
                    height: 28
                    z: 2
                    enabled: false
                    visible: mapPageID.mode === GuiController.MapMode.PlaceTour

                    Rectangle {
                        anchors.centerIn: parent
                        width: 18
                        height: 18
                        radius: 9
                        color: Colors.palette.accent
                        border.color: "white"
                        border.width: 2
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 6
                        height: 6
                        radius: 3
                        color: "white"
                    }
                }

                MapItemView {
                    id: tourStopsID

                    z: 1
                    model: TourController.GetTourStopsModel()
                    visible: mapPageID.mode === GuiController.MapMode.DisplayTour
                    delegate: MapQuickItem {
                        id: stopID

                        readonly property int itemSize: 20

                        z: 1
                        coordinate: QtPositioning.coordinate(model.Coords.latitude, model.Coords.longitude)
                        anchorPoint: Qt.point(stopID.itemSize / 2, stopID.itemSize / 2)
                        sourceItem: Rectangle {
                            implicitWidth: 20
                            implicitHeight: width
                            radius: width
                            color: model.Selected ? Colors.palette.selected : "white"
                            border {
                                color: Colors.palette.accentAlt
                                width: 1
                            }
                            Label {
                                anchors.centerIn: parent
                                text: model.SequenceNumber
                            }
                        }
                    }
                }

                MapItemView {
                    id: mapItemViewID

                    model: PastVuModelController.GetModel(ModelType.Clustered)
                    delegate: delegateID
                }

                Component {
                    id: delegateID

                    MapQuickItem {
                        id: mapItemID

                        readonly property int itemSize: 20

                        z: Selected ? 1 : 0
                        coordinate: model.Coordinate
                        anchorPoint: {
                            if (model.IsCluster) {
                                // Center anchor for clusters
                                return Qt.point(mapItemID.itemSize / 2, mapItemID.itemSize / 2)
                            } else {
                                // Bottom center anchor for individual markers (arrow points up)
                                return Qt.point(mapItemID.itemSize / 2, mapItemID.itemSize)
                            }
                        }

                        sourceItem: Loader {
                            id: itemLoaderID

                            property bool isCluster: model.IsCluster

                            width: mapItemID.itemSize
                            height: mapItemID.itemSize

                            sourceComponent: isCluster ? clusterMarkerID : individualMarkerID

                            Component {
                                id: individualMarkerID

                                PovDirection {
                                    id: povDirectionID

                                    size: mapItemID.itemSize
                                    bearing: model.Bearing - compassID.bearing
                                    mapBearing: mapID.bearing
                                    selected: model.Selected

                                    onClicked: mainWindowID.openPhotoDetails(model.Photo, model.Thumbnail, model.Title, model.Year)
                                }
                            }

                            Component {
                                id: clusterMarkerID

                                ClusterMarker {
                                    id: clusterMarkerIDInstance

                                    size: mapItemID.itemSize
                                    clusterCount: model.ClusterCount
                                    selected: model.Selected

                                    onClicked: {
                                        mapAnimationHelperID.animateMapCenterAndZoom(
                                            model.Coordinate,
                                            model.ZoomToDecluster,
                                            model.Coordinate
                                        )
                                    }
                                }
                            }
                        }
                    }
                }

                RecenterButton {
                    id: recenterID

                    anchors {
                        bottom: parent.bottom
                        left: parent.left
                        margins: 12
                    }

                    height: 48
                    width: 96

                    onTapped: {
                        if (mapPageID.mode === GuiController.MapMode.PlaceTour) {
                            if (positionSource.coordinate.isValid) {
                                mapID.center = positionSource.coordinate
                                mapID.updateViewCoordinates()
                            }
                        } else {
                            mapID.follow = true
                        }
                    }
                }

                // Drag to pan (continuous)
                DragHandler {
                    id: mapDragID

                    dragThreshold: 20
                    grabPermissions: PointerHandler.CanTakeOverFromAnything
                    xAxis.onActiveValueChanged: (dx) => target.pan(-dx, 0)
                    yAxis.onActiveValueChanged: (dy) => target.pan(0, -dy)

                    onActiveChanged: {
                        if (active)
                            mapID.follow = false
                        else
                            mapID.updateViewCoordinates()
                    }
                }

                PinchHandler {
                    id: pinchHandlerID

                    target: null
                    onActiveChanged: {
                        if (active)
                            mapID.follow = false
                        else
                            mapID.updateViewCoordinates()

                        mapID.startCentroid = mapID.toCoordinate(pinchHandlerID.centroid.position, false)
                    }
                    onScaleChanged: (delta) => {
                        mapID.zoomLevel += Math.log2(delta)
                        mapID.alignCoordinateToPoint(mapID.startCentroid, pinchHandlerID.centroid.position)
                    }
                    onRotationChanged: (delta) => {
                        mapID.bearing -= delta
                        mapID.alignCoordinateToPoint(mapID.startCentroid, pinchHandlerID.centroid.position)
                    }
                    grabPermissions: PointerHandler.TakeOverForbidden
                }

                TapHandler {
                    id: mapTapHandlerID

                    target: null
                    onDoubleTapped: {
                        mapID.follow = false
                        ++mapID.zoomLevel
                        mapID.center = mapID.toCoordinate(mapTapHandlerID.point.position)
                    }
                }

                MapQuickItem {
                    id: youAreHereMarkerID

                    anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height / 2)
                    coordinate: positionSource.coordinate
                    visible: coordinate.isValid
                    sourceItem: UserPosition {
                        bearing: positionSource.bearing
                        mapBearing: mapID.bearing
                    }
                }
            }
        }

        Loader {
            id: subAreaLoaderID

            Layout.fillWidth: true
            Layout.preferredHeight: mapPageID.mode === GuiController.MapMode.PlaceTour
                ? (item ? item.implicitHeight : 0) : 210
            Layout.margins: 10

            sourceComponent: {
                switch (mapPageID.mode) {
                    case GuiController.MapMode.Main: return photosNearID
                    case GuiController.MapMode.TourCreation: return tourCreationID
                    case GuiController.MapMode.PlaceTour: return placeTourID
                    case GuiController.MapMode.DisplayTour: return displayTourID
                    default: console.error("Unknown map mode.")
                }
            }

            Component {
                id: photosNearID

                PhotosNear {
                    map: mapID
                }
            }

            Component {
                id: placeTourID

                PlaceTourControls {
                    onAddPointRequested: TourController.AddPoint(mapID.center)
                    onAddStopRequested: mainWindowID.openAddTourStopDescription(mapID.center)
                    onFinishRequested: mainWindowID.finishPlacedTour()
                }
            }

            Component {
                id: tourCreationID

                TourInfoPanel {
                    mode: GuiController.TourInfoMode.Edit
                    stopCount: TourController.currentStops
                    distance: TourController.distance
                }
            }

            Component {
                id: displayTourID

                TourInfoPanel {
                    mode: GuiController.TourInfoMode.Tour
                    stopCount: TourController.currentTour.stopCount
                    distance: TourController.currentTour.distance
                }
            }
        }
    }
}
