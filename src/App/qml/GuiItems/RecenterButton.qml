import QtQuick
import QtQuick.Controls

import Colors

Rectangle {
    id: rootID

    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Re-center")
    Accessible.onPressAction: tapped()

    signal tapped()

    radius: width/2
    color: Colors.seeThroughBlack
    border {
        color: Colors.border
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