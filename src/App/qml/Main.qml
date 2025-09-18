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

            Map {
                id: map

                property geoCoordinate startCentroid

                anchors.fill: parent
                plugin: Plugin { name: "osm" }

                // Start somewhere sensible until GPS is valid:
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
                        coordinate: model.coordinate
                        sourceItem: Rectangle {
                            id: b
                            width: 40; height: 40; radius: 20
                            color: "#2b6cb0"; opacity: 0.9
                            border.width: 2; border.color: "white"
                            Text {
                                anchors.centerIn: parent
                                color: "white"
                                font.bold: true
                                text: model.title
                            }
                        }
                    }
                }

                // Drag to pan (continuous)
                DragHandler {
                    id: mapDrag
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
                    anchorPoint: Qt.point(dot.width/2, dot.height/2)
                    coordinate: positionSource.position.coordinate
                    visible: coordinate.isValid
                    sourceItem: Rectangle { id: dot; width: 14; height: 14; radius: 7; color: "#3dafff" }
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