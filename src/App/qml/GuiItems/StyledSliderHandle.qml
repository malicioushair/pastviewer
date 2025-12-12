import QtQuick

import "../Helpers/colors.js" as Colors

Rectangle {
    property bool pressed: false
    property real visualPosition: 0
    property real availableWidth: 0
    property real xOffset: 0
    property real yOffset: 0

    implicitWidth: 20
    implicitHeight: 20
    radius: 10
    color: pressed ? Colors.palette.accentAltPressed : Colors.palette.accentAlt

    x: visualPosition * availableWidth + xOffset
    y: -height / 4 + yOffset
    antialiasing: true
    border.color: Colors.palette.shadowSoft
    border.width: 1
}

