import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning

ApplicationWindow {
    width: 900; height: 600; visible: true
    title: "Past Viewer"

    // follow the user only until we get the first good fix (you can toggle back to true)
    property bool followMe: true
    property bool firstFixDone: false

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
                anchors.fill: parent
                plugin: Plugin { name: "osm" }

                // Start somewhere sensible until GPS is valid:
                center: positionSource.position.coordinate
                zoomLevel: 13

                // Wheel/trackpad zoom
                WheelHandler {
                    property: "zoomLevel"
                    rotationScale: 1/120
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
                    target: null
                    onScaleChanged: (delta) => {
                        map.zoomLevel += Math.log2(delta)
                        map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position)
                    }
                    onRotationChanged: (delta) => {
                        map.bearing -= delta
                        map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position)
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
            text: `position: ${positionSource.position.coordinate.isValid}\n lat/lon ${positionSource.position.coordinate.latitude}/${positionSource.position.coordinate.longitude}`
        }
    }
}