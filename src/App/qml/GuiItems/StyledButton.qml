import QtQuick
import QtQuick.Controls

import Analytics
import "../Helpers/colors.js" as Colors

Button {
    id: controlID

    property bool reversedColors: false
    property bool transparent: false
    property string analyticsId: "" // empty if not tracked

    Connections {
        target: controlID

        function onClicked() {
            if (controlID.analyticsId !== "")
                Analytics.Track("button_clicked", {
                    button: controlID.analyticsId
                })
        }
    }

    background: Rectangle {
    color: controlID.transparent
        ? "transparent"
        : !controlID.enabled
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
        color: !controlID.enabled ? Colors.palette.disabledBtnText
              : controlID.transparent ? Colors.palette.text : "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    padding: 12
    implicitHeight: 44
}
