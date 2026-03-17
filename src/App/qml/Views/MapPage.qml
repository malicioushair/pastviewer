import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Shapes

import PastViewer 1.0

import "../GuiItems"
import "../Helpers"

import "../Helpers/colors.js" as Colors
import "../Helpers/utils.js" as Utils

Item {
    id: mapPageID

    readonly property var positionSource: pastVuModelController.GetPositionSource()
    property alias map: mapViewID.internalMap

    Connections {
        target: guiController
        function onOnboardingReset() {
            mapOnboardingID.currentIndex = 0
            mapOnboardingID.active = !guiController.IsOnboardingStepCompleted(mapOnboardingID.completionKey)
        }
    }

    // Stack-level onboarding for the main map view
    OnboardingOverlay {
        id: mapOnboardingID

        anchors.fill: parent

        z: 1000

        completionKey: "MapPageIntro"
        active: !guiController.IsOnboardingStepCompleted(completionKey)
        centered: currentIndex === 3
        topped: currentIndex === 4 || currentIndex === 5

        highlightSteps: [
            { target: recenterID, stepIndex: 2 },
            { target: photosNearID, stepIndex: 3 },
            { target: compassID, stepIndex: 4 },
            { target: settingsButtonID, stepIndex: 5 },
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
                    right: parent.right
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
                    left: parent.left
                    margins: 12
                }

                z: 999   // above the map

                onClicked: mainWindowID.openSettings()
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
                    pastVuModelController.SetViewportCoordinates(QtPositioning.rectangle(topLeftCoord, bottomRightCoord))
                }

                anchors.fill: parent

                copyrightsVisible: false
                onBearingChanged: updateViewCoordinates()

                plugin: Plugin {
                    id: mapPluginID

                    name: "osm"

                    PluginParameter {
                        name: "osm.mapping.custom.host"
                        value: "https://tiles.stadiamaps.com/tiles/outdoors/%z/%x/%y.png?api_key=" + pastVuModelController.GetMapHostApiKey()
                    }
                }

                activeMapType: supportedMapTypes.find((map) => { return map.style === MapType.CustomMap })

                onZoomLevelChanged: (zoomLevel) => { pastVuModelController.zoomLevel = zoomLevel }
                Component.onCompleted: {
                    zoomLevel = pastVuModelController.zoomLevel
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

                        mapID.center =  QtPositioning.coordinate(mainWindowID.mapAnimationHelper.animatedLat, mainWindowID.mapAnimationHelper.animatedLon)
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

                MapItemView {
                    id: mapItemViewID

                    model: pastVuModelController.GetModel(ModelType.Clustered)
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
                        right: parent.right
                        margins: 12
                    }

                    height: 48
                    width: 96

                    onTapped: mapID.follow = true
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

        PhotosNear {
            id: photosNearID
            map: mapID
        }
    }
}