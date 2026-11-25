import QtQuick
import QtQuick.Controls

import Colors

Dialog {
    modal: true
    focus: true
    width: Math.min(parent.width * 0.9, 420)
    padding: 20
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        radius: 16
        color: Colors.toolbar
        border.color: Colors.border
        border.width: 1
    }
}
