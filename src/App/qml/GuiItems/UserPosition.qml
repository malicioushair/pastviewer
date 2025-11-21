import QtQuick
import QtQuick.Shapes

import "../Helpers/colors.js" as Colors

Item {
    id: rootID

    required property real bearing
    required property real mapBearing

    width: 14
    height: width

    Loader {
        anchors.fill: parent
        sourceComponent: isNaN(bearing) ? noDirectionMarkerID : directionMarkerID
    }

    Component {
        id: noDirectionMarkerID

        Rectangle {
            anchors.fill: parent

            radius: width
            color: Colors.palette.userPosition
        }
    }

    Component {
        id: directionMarkerID

        Item {
            id: arrowContainerID

            property real bearing: rootID.bearing
            property real mapBearing: rootID.mapBearing

            anchors.fill: parent

            transform: Rotation {
                origin.x: arrowContainerID.width / 2
                origin.y: arrowContainerID.height / 2
                angle: isNaN(arrowContainerID.bearing) ? 0 : ((arrowContainerID.bearing - arrowContainerID.mapBearing) % 360 + 360) % 360
            }

            Shape {
                id: arrowHeadID

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top

                width: rootID.width
                height: width

                ShapePath {
                    fillColor: Colors.palette.userPosition
                    strokeColor: "transparent"
                    PathPolyline {
                        path: [
                            Qt.point(arrowHeadID.width / 2, 0),
                            Qt.point(0, arrowHeadID.height),
                            Qt.point(arrowHeadID.width / 2, arrowHeadID.height / 1.5),
                            Qt.point(arrowHeadID.width, arrowHeadID.height),
                            Qt.point(arrowHeadID.width / 2, 0)
                        ]
                    }
                }
            }
        }
    }
}