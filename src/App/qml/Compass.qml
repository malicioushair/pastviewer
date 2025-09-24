import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Item {
    id: compassID

    readonly property alias bearing: needleID.rotation

    Rectangle {
        id: dialID

        anchors.fill: parent
        radius: width/2
        color: "#66000000"
        border {
            color: "white"
            width: 1
        }
    }

    // needle rotates opposite the map's bearing to keep N up on screen
    Item {
        id: needleHolderID

        anchors.fill: parent
        transform: Rotation {
            origin {
                x: needleHolderID.width / 2
                y: needleHolderID.height / 2
            }
            angle: -map.bearing
        }

        Shape {
            id: needleID

            anchors.fill: parent
            layer.enabled: true

            ShapePath {
                id: northID

                strokeWidth: 0
                fillColor: "#e53935"
                startX: needleID.x + needleID.width / 2
                startY: needleID.y
                PathLine {
                    x: needleID.x + needleID.width * 0.35
                    y: needleID.y + needleID.height / 2
                }
                PathLine {
                    x: needleID.x + needleID.width * 0.65
                    y: needleID.y + needleID.height / 2
                }
            }

            ShapePath {
                id: southID

                strokeWidth: 0
                fillColor: "white"
                startX: needleID.x + needleID.width / 2
                startY: needleID.y + needleID.height
                PathLine {
                    x: needleID.x + needleID.width * 0.35
                    y: needleID.y + needleID.height / 2
                }
                PathLine {
                    x: needleID.x + needleID.width * 0.65
                    y: needleID.y + needleID.height / 2
                }
            }
        }

        Text {
            text: "N"

            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                topMargin: 8
            }
            color: "white"
            font.pixelSize: 12
        }

    }

    TapHandler {
        onTapped: map.bearing = 0  // tap to snap north-up
    }
}