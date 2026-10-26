import QtQuick
import QtQuick.Controls

import "../Helpers/colors.js" as Colors

Button {
    id: controlID

    property bool reversedColors: false

    background: Rectangle {
    color: !controlID.enabled
        ? Colors.palette.disabledBtnBg
        : controlID.reversedColors
            ? !controlID.pressed
                ? Colors.palette.accentAlt
                : Colors.palette.accent
            : controlID.pressed
                ? Colors.palette.accentAlt
                : Colors.palette.accent

        radius: 8
        border {
            color: controlID.enabled ? Colors.palette.border : Colors.palette.disabledBtnBorder
            width: 2
        }

        Behavior on color {
            ColorAnimation {
                duration: 150
            }
        }
    }

    contentItem: Text {
        text: controlID.text
        font {
            pixelSize: 16
            bold: true
        }
        color: controlID.enabled ? "white" : Colors.palette.disabledBtnText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    padding: 12
    implicitHeight: 44
}