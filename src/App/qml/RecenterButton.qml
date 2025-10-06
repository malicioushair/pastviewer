import QtQuick
import QtQuick.Controls

Rectangle {
    id: rootID

    signal tapped()

    radius: width/2
    color: "#66000000"
    border {
        color: "#313233"
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