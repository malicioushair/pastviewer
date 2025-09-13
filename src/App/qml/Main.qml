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
            onActiveChanged:    console.log("active:", active)
            onSourceErrorChanged: console.log("error:", error) // 0 OK, 1 AccessError, 2 ClosedError, etc.
            onPositionChanged: {
                const c = position.coordinate
                console.log("pos changed, valid?", c.isValid, "lat/lon:", c.latitude, c.longitude)
            }
            Component.onCompleted: {
                console.log("valid backend?", valid,
                            "supported:", supportedPositioningMethods)
            }
        }

        MapView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Map {
                id: map
                anchors.fill: parent
                plugin: Plugin { name: "osm" }

                // Start somewhere sensible until GPS is valid:
                center: QtPositioning.coordinate(44.812, 20.461)  // Belgrade
                zoomLevel: 13

                // Wheel/trackpad zoom
                WheelHandler { target: map; property: "zoomLevel"; rotationScale: 1/120 }

                // Drag to pan (continuous)
                DragHandler {
                    id: mapDrag
                    target: null
                    dragThreshold: 0
                    grabPermissions: PointerHandler.CanTakeOverFromAnything
                    xAxis.onActiveValueChanged: (dx) => map.pan(-dx, 0)
                    yAxis.onActiveValueChanged: (dy) => map.pan(0, -dy)
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
    }
}