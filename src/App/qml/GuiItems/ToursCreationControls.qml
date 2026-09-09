import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtLocation
import QtPositioning

import PastViewer 1.0

import "../Helpers/colors.js" as Colors
import "../Helpers/utils.js" as Utils

Rectangle {
    id: rootID

    Layout.fillWidth: true
    Layout.preferredHeight: 210

    radius: 16
    color: Colors.palette.bg

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }
        spacing: 10

        Text {
            text: qsTr("Creating a tour")
            font.bold: true
            font.pixelSize: 18
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                Layout.fillWidth: true

                InfoLabel {
                    id: distanceInfoID

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    infoText: qsTr("Distance")
                    valueText: "0 m"
                }

                InfoLabel {
                    id: stopsID

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    infoText: qsTr("Stops")
                    valueText: "0"
                }
            }

            RowLayout {
                Layout.fillWidth: true

                StyledButton {
                    id: recID

                    Layout.fillWidth: true
                    text: qsTr("Start\n recording")
                }

                StyledButton {
                    id: addStopID

                    Layout.fillWidth: true
                    text: qsTr("Add stop")
                }
            }
        }
    }
}
