import QtQuick
import QtQuick.Controls

Rectangle {
    id: rootID

    radius: width/2
    color: "#66000000"
    border {
        color: "white"
        width: 1
    }

    Text {
        anchors.centerIn: parent

        text: qsTr("Re-center")
        color: "white"
    }

    TapHandler {
        onTapped: positionSourceID.active = true
    }
}