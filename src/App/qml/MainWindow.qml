import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

import PastViewer 1.0

import "Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    color: Colors.palette.bg

    StackView {
        id: stackViewID

        function openPhotoDetails(photo, title, year) {
            stackViewID.push("PhotoDetails.qml", {
                imageSource: photo,
                title: title,
                year: year
            })
        }

        anchors.fill: parent

        initialItem: mapPageID
    }

    ErrorDialog {
        id: errorDialogID

        anchors.centerIn: Overlay.overlay
    }

    Connections {
        target: guiController

        function onShowErrorDialog(message) {
            errorDialogID.errorMessage = message
            errorDialogID.open()
        }
    }

    Component {
        id: mapPageID

        ColumnLayout {
            readonly property var positionSource: pastVuModelController.GetPositionSource()

            anchors.fill: parent

            MapView {
                id: mapViewID

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

                Rectangle {
                    id: imagesNearbyID

                    anchors {
                        top: parent.top
                        left: parent.left
                        margins: 12
                    }

                    width: 24
                    height: 24
                    z: 999

                    radius: 10

                    color: Colors.palette.accentAlt
                    border.color: Colors.palette.border

                    Text {
                        anchors.centerIn: parent
                        text: mapItemViewID.model.count
                        color: "white"
                    }
                }

                Map {
                    id: mapID

                    property geoCoordinate startCentroid
                    property bool follow: true

                    anchors.fill: parent
                    copyrightsVisible: false
                    plugin: Plugin {
                        id: mapPluginID

                        name: "osm"

                        PluginParameter {
                            name: "osm.mapping.custom.host"
                            value: "https://tiles.stadiamaps.com/tiles/outdoors/%z/%x/%y.png?api_key=" + pastVuModelController.GetMapHostApiKey()
                        }
                    }

                    activeMapType: {
                        const customMapType = supportedMapTypes.find((map) => { return map.style === MapType.CustomMap });
                        if (customMapType)
                            return customMapType;
                        else
                            console.warn("CustomMap not provided by this plugin.");
                    }

                    zoomLevel: 13

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

                        model: pastVuModelController.GetModel()
                        delegate: delegateID
                    }

                    Component {
                        id: delegateID

                        MapQuickItem {
                            z: Selected ? 1 : 0
                            coordinate: model.Coordinate
                            anchorPoint: Qt.point(povDirectionID.width / 2, povDirectionID.height)

                            sourceItem: PovDirection {
                                id: povDirectionID

                                size: 20
                                bearing: model.Bearing - compassID.bearing
                                mapBearing: mapID.bearing
                                selected: model.Selected

                                onClicked: stackViewID.openPhotoDetails(model.Photo, model.Title, model.Year)
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
                            if (active) {
                                mapID.follow = false
                            }
                        }
                    }

                    PinchHandler {
                        id: pinchHandlerID

                        target: null
                        onActiveChanged: {
                            if (active)
                                mapID.follow = false
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

                    // "you are here" marker
                    MapQuickItem {
                        anchorPoint: Qt.point(dotID.width / 2, dotID.height / 2)
                        coordinate: positionSource.coordinate
                        visible: coordinate.isValid
                        sourceItem: Rectangle {
                            id: dotID

                            width: 14
                            height: 14
                            radius: 7
                            color: "#3dafff"
                        }
                    }
                }
            }

            PhotosNear {
                id: photosNearID

            }
        }
    }
}
