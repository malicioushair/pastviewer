import QtQuick
import QtQuick.Shapes

import "../Helpers/colors.js" as Colors

Item {
    id: rootID

    required property real bearing
    required property real mapBearing

    width: 14
    height: width

    Rectangle {
        visible: isNaN(rootID.bearing)
        width: parent.width
        height: parent.height
        radius: width
        color: Colors.palette.userPosition
    }

    Item {
        id: arrowContainerID

        visible: !isNaN(rootID.bearing)
        width: parent.width
        height: parent.height

        transform: Rotation {
            origin.x: arrowContainerID.width / 2
            origin.y: arrowContainerID.height / 2
            angle: isNaN(rootID.bearing) ? 0 : ((rootID.bearing - rootID.mapBearing) % 360 + 360) % 360
        }

        Shape {
            id: arrowHeadID

            width: parent.width
            height: parent.height

            ShapePath {
                fillColor: Colors.palette.userPosition
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
