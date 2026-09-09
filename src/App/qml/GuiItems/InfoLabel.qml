import QtQuick
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors

Rectangle {
    id: distanceID

    property alias infoText: infoTextID.text
    property alias valueText: valueTextID.text

    color: Colors.palette.selected
    radius: 10

    border {
        color: Colors.palette.border
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Text {
            id: infoTextID

            Layout.topMargin: 10
            Layout.alignment: Qt.AlignCenter

            text: qsTr("Distance")
        }
        Text {
            id: valueTextID

            Layout.bottomMargin: 20
            Layout.alignment: Qt.AlignCenter

            text: qsTr("0 m")
        }
    }
}