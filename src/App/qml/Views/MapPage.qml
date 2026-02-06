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

ColumnLayout {
    id: columnLayoutID

    readonly property var positionSource: pastVuModelController.GetPositionSource()
    property alias map: mapViewID.internalMap

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

            function scheduleViewUpdate() {
                mapMovementTimerID.restart()
            }

            function updateViewCoordinates() {
                if (mapID.width <= 0 || mapID.height <= 0)
                    return

                const topLeftCoord = mapID.toCoordinate(Qt.point(0, 0), false)
                const bottomRightCoord = mapID.toCoordinate(Qt.point(mapID.width, mapID.height), false)
                pastVuModelController.SetViewportCoordinates(QtPositioning.rectangle(topLeftCoord, bottomRightCoord))
            }

            anchors.fill: parent

            copyrightsVisible: false
            zoomLevel: pastVuModelController.zoomLevel
            onBearingChanged: scheduleViewUpdate()

            plugin: Plugin {
                id: mapPluginID

                name: "osm"

                PluginParameter {
                    name: "osm.mapping.custom.host"
                    value: "https://tiles.stadiamaps.com/tiles/outdoors/%z/%x/%y.png?api_key=" + pastVuModelController.GetMapHostApiKey()
                }
            }

            activeMapType: supportedMapTypes.find((map) => { return map.style === MapType.CustomMap })
            onZoomLevelChanged: pastVuModelController.zoomLevel = zoomLevel

            Component.onCompleted: scheduleViewUpdate()

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

            Timer {
                id: mapMovementTimerID
                interval: 300  // Wait 300ms after map stops moving // @TODO figure out how to know when the pan is stopped // emit signal mb?
                onTriggered: mapID.updateViewCoordinates()
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

                                onClicked: mainWindowID.openPhotoDetails(model.Photo, model.Title, model.Year)
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
                        mapID.scheduleViewUpdate()
                }
            }

            PinchHandler {
                id: pinchHandlerID

                target: null
                onActiveChanged: {
                    if (active)
                        mapID.follow = false
                    else
                        mapID.scheduleViewUpdate()

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

