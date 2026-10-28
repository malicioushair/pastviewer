import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    Accessible.role: Accessible.Button
    Accessible.name: "Settings"
    Accessible.onPressAction: clicked()

    signal clicked()

    width: 40
    height: 40
    radius: 10

    color: Colors.palette.accentAlt

    ColumnLayout{
        id: burgerMenu
        anchors.centerIn: parent

        Repeater {
            model: 3

            Rectangle {
                width: 25
                height: 2

                color: Colors.palette.toolbar
            }
        }
    }

    TapHandler {
        onTapped: clicked()
    }
}
