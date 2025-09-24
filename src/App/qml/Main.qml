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
                id: positionSource
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
                    id: map

                    property geoCoordinate startCentroid

                    anchors.fill: parent
                    plugin: Plugin { name: "osm" }

                    // @todo Start somewhere sensible until GPS is valid:
                    center: positionSource.position.coordinate
                    zoomLevel: 13

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
                                mapBearing: map.bearing

                                onClicked: {
                                    print("DELEGATE_CLICKED")

                                    stackViewID.push("PhotoDetails.qml", {
                                        imageSource: model.File,
                                        title: model.Title,
                                        year: model.Year
                                    })
                                }
                            }
                        }
                    }

                    // Drag to pan (continuous)
                    DragHandler {
                        id: mapDragID

                        dragThreshold: 0
                        grabPermissions: PointerHandler.CanTakeOverFromAnything
                        xAxis.onActiveValueChanged: (dx) => target.pan(-dx, 0)
                        yAxis.onActiveValueChanged: (dy) => target.pan(0, -dy)
                    }

                    PinchHandler {
                        id: pinchHandlerID

                        target: null
                        onActiveChanged: if (active) {
                            map.startCentroid = map.toCoordinate(pinchHandlerID.centroid.position, false)
                        }
                        onScaleChanged: (delta) => {
                            map.zoomLevel += Math.log2(delta)
                            map.alignCoordinateToPoint(map.startCentroid, pinchHandlerID.centroid.position)
                        }
                        onRotationChanged: (delta) => {
                            map.bearing -= delta
                            map.alignCoordinateToPoint(map.startCentroid, pinchHandlerID.centroid.position)
                        }
                        grabPermissions: PointerHandler.TakeOverForbidden
                    }

                    // "you are here" marker (optional)
                    MapQuickItem {
                        anchorPoint: Qt.point(dotID.width / 2, dotID.height / 2)
                        coordinate: positionSource.position.coordinate
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
                text: `position: ${positionSource.position.coordinate.isValid}\n
                lat/lon ${positionSource.position.coordinate.latitude}/${positionSource.position.coordinate.longitude}\n
                number of photos nearby: ${pastVuModelController.GetModel().rowCount()}
                `
            }
        }
    }
}