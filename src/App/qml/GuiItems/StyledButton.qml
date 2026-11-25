import QtQuick
import QtQuick.Controls

import Analytics
import Colors

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
        ? Colors.disabledBtnBg
        : controlID.reversedColors
            ? !controlID.pressed
                ? Colors.accentAlt
                : Colors.accent
            : controlID.pressed
                ? Colors.accentAlt
                : Colors.accent

        radius: 8
        border {
            color: controlID.enabled ? Colors.border : Colors.disabledBtnBorder
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
        color: !controlID.enabled ? Colors.disabledBtnText
              : controlID.transparent ? Colors.text : "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    padding: 12
    implicitHeight: 44
}
