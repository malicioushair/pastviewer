import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors

CheckBox {
    id: controlID
    
    indicator: Rectangle {
        implicitWidth: 24
        implicitHeight: 24
        x: controlID.leftPadding
        y: parent.height / 2 - height / 2
        radius: 6
        color: controlID.checked ? Colors.palette.accent : Colors.palette.bg
        border {
            color: controlID.checked ? Colors.palette.accent : Colors.palette.border
            width: 2
        }
        
        Rectangle {
            anchors.centerIn: parent
            width: 8
            height: 8
            radius: 4
            color: Colors.palette.bg
            visible: controlID.checked
        }
    }
    
    contentItem: Text {
        text: controlID.text
        font.pixelSize: 16
        color: Colors.palette.text
        leftPadding: controlID.indicator.width + controlID.spacing
        verticalAlignment: Text.AlignVCenter
    }
}

