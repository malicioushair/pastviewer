import QtQuick
import QtQuick.Layouts

import Colors

Rectangle {
    id: rootID

    required property string infoText
    required property string valueText

    implicitWidth: contentLayoutID.implicitWidth
    implicitHeight: contentLayoutID.implicitHeight

    color: Colors.selected
    radius: 10

    border {
        color: Colors.border
    }

    ColumnLayout {
        id: contentLayoutID

        anchors.fill: parent
        spacing: 0

        Text {
            id: infoTextID

            Layout.topMargin: 10
            Layout.alignment: Qt.AlignCenter

            text: rootID.infoText
        }
        Text {
            id: valueTextID

            Layout.bottomMargin: 20
            Layout.alignment: Qt.AlignCenter

            text: rootID.valueText
        }
    }
}