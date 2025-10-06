import QtQuick
import QtQuick.Shapes

Rectangle {
    id: rootID

    required property int size
    required property real bearing // degrees, 0=N, 90=E (clockwise)
    required property real mapBearing

    property bool selected: false

    signal clicked()

    property color arrowColor: "black"

    width: size
    height: size

    radius: width / 2
    color: selected ? "green" : "white"
    border.color: "#313233"

    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds | TapHandler.WithinBounds
        onTapped: rootID.clicked()
    }

    Item {
        id: arrowID

        anchors.fill: parent

        transform: Rotation {
            origin.x: arrowID.width / 2
            origin.y: arrowID.height / 2
            angle: ((bearing - mapBearing) % 360 + 360) % 360
        }

        Rectangle {
            id: arrowShaftID

            width: 2
            height: rootID.height * 0.42
            color: rootID.arrowColor

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: -rootID.height * 0.08

            antialiasing: true
        }

        Shape {
            id: arrowHeadID

            width: rootID.height * 0.30
            height: width

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: rootID.height * 0.14

            ShapePath {
                fillColor: rootID.arrowColor
                strokeColor: "transparent"
                PathMove {
                    x: arrowHeadID.width / 2
                    y: 0
                }
                PathLine {
                    x: 0
                    y: arrowHeadID.height
                }
                PathLine {
                    x: arrowHeadID.width
                    y: arrowHeadID.height
                }
            }
        }
    }
}