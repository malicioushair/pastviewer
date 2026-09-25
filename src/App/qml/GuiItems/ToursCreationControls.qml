import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtLocation
import QtPositioning

import PastViewer 1.0
import TourController 1.0

import "../Helpers/colors.js" as Colors

import TourController

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
                    valueText: TourController.distance
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

                    property bool inProgress: false
                    property bool freshRecording: true

                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width / 2

                    text: {
                        if (freshRecording)
                            return qsTr("Start recording")
                        else if (inProgress)
                            return qsTr("Pause")
                        else
                            return qsTr("Unpause")
                    }
                    reversedColors: inProgress
                    onClicked: {
                        if (freshRecording) {
                            TourController.StartRecording()
                            freshRecording = false
                        }
                        else if (inProgress)
                            TourController.PauseRecording()
                        else
                            TourController.UnpauseRecording()

                        inProgress = !inProgress
                    }
                }

                StyledButton {
                    id: addStopID

                    Layout.fillWidth: true
                    Layout.preferredWidth: parent.width / 2

                    text: qsTr("Add stop")
                    onClicked: mainWindowID.openAddTourStopDescription()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                Item {
                    Layout.fillWidth: true
                }

                StyledButton {
                    id: publishButtonID

                    text: qsTr("Finish")

                    onClicked: {
                        TourController.StopRecording()
                        mainWindowID.openDrafts()
                        mapPageID.mode = GuiController.MapMode.Main
                    }
                }
            }
        }
    }
}
