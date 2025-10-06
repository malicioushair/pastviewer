import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

Rectangle {
    id: rootID

    color: "#F5F0E5"

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

    Component {
        id: mapPageID

        ColumnLayout {
            anchors.fill: parent

            PositionSource {
                id: positionSourceID

                active: true
            }

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

                    color: "#6289ac"
                    border.color: "#313233"

                    Text {
                        anchors.centerIn: parent
                        text: mapItemViewID.model.rowCount()
                        color: "white"
                    }
                }

                Map {
                    id: mapID

                    property geoCoordinate startCentroid
                    property bool follow: true

                    anchors.fill: parent
                    plugin: Plugin { name: "osm" }

                    zoomLevel: 13

                    Component.onCompleted: {
                        if (positionSourceID.position.coordinate.isValid)
                            mapID.center = positionSourceID.position.coordinate
                    }

                    Binding {
                        id: followCenterID

                        target: mapID
                        property: "center"
                        value: positionSourceID.position.coordinate
                        when: mapID.follow && positionSourceID.position.coordinate.isValid
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
                            bottomMargin: 30
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

                    // "you are here" marker
                    MapQuickItem {
                        anchorPoint: Qt.point(dotID.width / 2, dotID.height / 2)
                        coordinate: positionSourceID.position.coordinate
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

                color: rootID.color
            }
        }
    }
}
