import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import TourController 1.0

import "../Helpers"
import "../Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    signal addPointRequested()
    signal addStopRequested()
    signal finishRequested()

    implicitHeight: panelLayoutID.implicitHeight + 24

    radius: 16
    color: Colors.palette.bg
    border.color: Colors.palette.border
    border.width: 1

    ColumnLayout {
        id: panelLayoutID

        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 12
        }
        spacing: 10

        Text {
            Layout.alignment: Qt.AlignHCenter

            text: qsTr("Place the path")
            color: Colors.palette.text
            font.bold: true
            font.pixelSize: 18
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            InfoLabel {
                Layout.fillWidth: true

                infoText: qsTr("Distance")
                valueText: TourController.distance
            }

            InfoLabel {
                Layout.fillWidth: true

                infoText: qsTr("Stops")
                valueText: TourController.currentStops
            }
        }

        StyledButton {
            Layout.fillWidth: true

            text: qsTr("Add point")
            onClicked: rootID.addPointRequested()
        }

        StyledButton {
            Layout.fillWidth: true

            text: qsTr("Remove last point")
            enabled: TourController.tourPath.length > 0
            opacity: enabled ? 1 : 0.45
            onClicked: removeLastPointDialogID.open()
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledButton {
                Layout.fillWidth: true

                text: qsTr("Add stop")
                onClicked: rootID.addStopRequested()
            }

            StyledButton {
                Layout.fillWidth: true

                text: qsTr("Finish")
                onClicked: rootID.finishRequested()
            }
        }
    }

    PromptDialog {
        id: removeLastPointDialogID

        anchors.centerIn: Overlay.overlay

        contentItem: ColumnLayout {
            spacing: 16

            Text {
                Layout.fillWidth: true

                text: qsTr("Are you sure you want to delete the last turnpoint? Removing it may also remove a stop at the same location.")
                wrapMode: Text.WordWrap
                color: Colors.palette.text
                font.pixelSize: 16
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8

                StyledButton {
                    text: qsTr("Cancel")
                    onClicked: removeLastPointDialogID.close()
                }

                StyledButton {
                    text: qsTr("Delete")
                    onClicked: {
                        TourController.RemoveLastPoint()
                        removeLastPointDialogID.close()
                    }
                }
            }
        }
    }
}
