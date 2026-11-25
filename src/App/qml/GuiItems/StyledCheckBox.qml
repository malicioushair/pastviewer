import QtQuick
import QtQuick.Controls

import Colors

CheckBox {
    id: controlID

    indicator: Rectangle {
        implicitWidth: 24
        implicitHeight: 24
        x: controlID.leftPadding
        y: (controlID.height - height) / 2
        radius: 5
        color: controlID.checked ? Colors.accent : Colors.bg
        border {
            color: controlID.checked ? Colors.accent : Colors.border
            width: 2
        }

        Rectangle {
            anchors.centerIn: parent
            width: 8
            height: 8
            radius: 4
            color: Colors.bg
            visible: controlID.checked
        }
    }

    contentItem: Text {
        text: controlID.text
        font.pixelSize: 16
        color: Colors.text
        leftPadding: controlID.indicator.width + controlID.spacing
        verticalAlignment: Text.AlignVCenter
    }
}
