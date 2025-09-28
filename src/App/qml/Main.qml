import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

ApplicationWindow {
    width: 900
    height: 600
    visible: true

    title: "Past Viewer"

    StackView {
        id: stackViewID

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
                            coordinate: model.Coordinate
                            anchorPoint: Qt.point(povDirectionID.width / 2, povDirectionID.height)

                            sourceItem: PovDirection {
                                id: povDirectionID

                                size: 20
                                bearing: model.Bearing - compassID.bearing
                                mapBearing: mapID.bearing

                                onClicked: {
                                    stackViewID.push("PhotoDetails.qml", {
                                        imageSource: model.File,
                                        title: model.Title,
                                        year: model.Year
                                    })
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

                    // "you are here" marker (optional)
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

            Text {
                text: `position: ${positionSourceID.position.coordinate.isValid}\n
                lat/lon ${positionSourceID.position.coordinate.latitude}/${positionSourceID.position.coordinate.longitude}\n
                number of photos nearby: ${pastVuModelController.GetModel().rowCount()}
                mapID.follow: ${mapID.follow}
                `
            }
        }
    }
}