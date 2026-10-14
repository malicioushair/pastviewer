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

    required property int mode
    required property int distance
    required property string stopCount

    property var tourStop: null

    Layout.fillWidth: true
    Layout.preferredHeight: 210

    radius: 16
    color: Colors.palette.bg

    ColumnLayout {
        anchors.fill: parent

        spacing: 10

        Text {
            Layout.alignment: Qt.AlignHCenter

            text: {
                switch(rootID.mode) {
                    case GuiController.TourInfoMode.Edit: return qsTr("Creating a tour")
                    case GuiController.TourInfoMode.Tour: return qsTr(TourController.currentTour.title)
                    default: return ""
                }
            }
            font.bold: true
            font.pixelSize: 18
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Loader {
                id: controlsLoaderID

                Layout.fillWidth: true
                Layout.fillHeight: true

                sourceComponent: {
                    switch (rootID.mode) {
                        case GuiController.TourInfoMode.Info:
                        case GuiController.TourInfoMode.Edit: return infoControlsID
                        case GuiController.TourInfoMode.Tour: return tourCountrolsID
                        default: {
                            console.warn("Unknown TourInfoMode")
                            return null
                        }
                    }

                }
            }

            Component {
                id: tourCountrolsID

                ColumnLayout {
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        InfoLabel {
                            id: nextStopID

                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            infoText: qsTr("Next Stop")
                            valueText: TourController.nextStopTitle

                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        StyledButton {
                            id: prevStopButtonID

                            text: "<"
                            onClicked: TourController.HighlightPrevStop()
                        }

                        StyledButton {
                            id: nextStopButtonID

                            text: ">"
                            onClicked: TourController.HighlightNextStop()
                        }

                        Item {
                            Layout.fillHeight: true
                            Layout.fillWidth: true
                        }

                        Connections {
                            target: TourController
                            function onSelectedStopChanged(stop) {
                                rootID.tourStop = stop
                            }
                        }

                        StyledButton {
                            id: imHereButtonID

                            text: qsTr("I'm here")
                            onClicked: mainWindowID.openTourStopInfo(rootID.tourStop)
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }

                        Label {
                            id: exitTourID

                            text: qsTr("Exit Tour")
                            TapHandler {
                                onTapped: mapPageID.mode = GuiController.MapMode.Main
                            }
                        }
                    }
                }
            }

            Component {
                id: infoControlsID
                RowLayout {
                    InfoLabel {
                        id: distanceInfoID

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        infoText: qsTr("Distance")
                        valueText: rootID.distance
                    }

                    InfoLabel {
                        id: stopsID

                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        infoText: qsTr("Stops")
                        valueText: rootID.stopCount
                    }
                }
            }


            // Buttons for tour recording
            RowLayout {
                Layout.fillWidth: true

                visible: rootID.mode == GuiController.TourInfoMode.Edit

                StyledButton {
                    id: recID

                    property bool inProgress: false
                    property bool freshRecording: true

                    Layout.fillWidth: true

                    text: {
                        if (freshRecording)
                            return qsTr("Start recording")
                        else if (inProgress)
                            return qsTr("Pause")
                        else
                            return qsTr("Resume")
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
                            TourController.ResumeRecording()

                        inProgress = !inProgress
                    }
                }

                StyledButton {
                    id: addStopID

                    Layout.fillWidth: true

                    text: qsTr("Add stop")
                    onClicked: mainWindowID.openAddTourStopDescription()
                }
            }

            // Save tour
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                visible: mode == GuiController.TourInfoMode.Edit

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
