import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "Helpers"

import TourController

BasePage {
    id: rootID

    required property var tourStop

    title: qsTr("Stop Details")

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }

        Image {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            source: rootID.tourStop.imageUrl
            fillMode: Image.PreserveAspectFit

            TapHandler {
                onDoubleTapped: {
                    mainWindowID.openInspectPhotos(parent.source)
                }
            }
        }

        Label {
            Layout.fillWidth: true

            text: rootID.tourStop.title
            font.bold: true
            font.pixelSize: 20
        }

        Label {
            Layout.fillWidth: true

            text: rootID.tourStop.description
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Item {
                Layout.fillWidth: true
            }
            StyledButton {
                Layout.preferredHeight: 50

                text: qsTr(">")
                onClicked: {
                    rootID.goBack()
                    TourController.HighlightNextStop()
                }
            }
        }
    }
}