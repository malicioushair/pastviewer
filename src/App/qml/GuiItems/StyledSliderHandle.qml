import QtQuick

import "../Helpers/colors.js" as Colors

Rectangle {
    required property var control

    property bool pressed: false
    property real visualPosition: 0
    property real xOffset: 0
    property real yOffset: 0

    implicitWidth: 20
    implicitHeight: 20
    radius: 10
    color: pressed ? Colors.palette.accentAltPressed : Colors.palette.accentAlt

    x: control.leftPadding + visualPosition * (control.availableWidth - width) + xOffset
    y: control.topPadding + (control.availableHeight - height) / 2 + yOffset
    antialiasing: true
    border.color: Colors.palette.shadowSoft
    border.width: 1
}
