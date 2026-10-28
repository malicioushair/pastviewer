import QtQuick
import QtQuick.Controls

import "../Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Re-center")
    Accessible.onPressAction: tapped()

    signal tapped()

    radius: width/2
    color: Colors.palette.seeThroughBlack
    border {
        color: Colors.palette.border
        width: 1
    }

    Text {
        anchors.centerIn: parent

        text: qsTr("Re-center")
        color: "white"
    }

    TapHandler {
        grabPermissions: PointerHandler.TakeOverForbidden
        onTapped: rootID.tapped()
    }
}