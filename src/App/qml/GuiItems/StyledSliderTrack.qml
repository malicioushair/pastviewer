import QtQuick

import "../Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    required property var control
    property real fillFrom: 0
    property real fillTo: 0

    x: control.leftPadding
    y: control.topPadding + (control.availableHeight - height) / 2
    width: control.availableWidth
    implicitHeight: 8
    height: implicitHeight
    radius: 4
    color: Colors.palette.sliderAlt

    Rectangle {
        x: rootID.fillFrom * parent.width
        width: Math.max(0, rootID.fillTo - rootID.fillFrom) * parent.width
        height: parent.height
        radius: parent.radius
        color: Colors.palette.slider
    }
}
