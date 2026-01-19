import QtQuick
import QtQuick.Shapes

import "../Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    required property int size
    required property int clusterCount

    property bool selected: false

    signal clicked()

    width: size
    height: size

    radius: width / 2
    color: selected ? Colors.palette.selected : Colors.palette.accent
    border.color: Colors.palette.border
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

