import QtQuick
import QtQuick.Shapes

Rectangle {
    id: rootID

    required property int size
    required property int clusterCount

    property bool selected: false

    signal clicked()

    width: size
    height: size

    radius: width / 2
    color: selected ? "#b7b2a7" : "#C2643F"
    border.color: "#313233"
    border.width: 2

    TapHandler {
        gesturePolicy: TapHandler.ReleaseWithinBounds | TapHandler.WithinBounds
        onTapped: rootID.clicked()
    }

    Text {
        id: countTextID

        anchors.centerIn: parent
        text: rootID.clusterCount
        color: "white"
        font.pixelSize: Math.max(10, rootID.size * 0.4)
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}

