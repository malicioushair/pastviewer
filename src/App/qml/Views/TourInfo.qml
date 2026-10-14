import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "Helpers"

import PastViewer
import TourController

BasePage {
    id: rootID

    required property var tour

    title: qsTr("Tour Info")

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
            Layout.preferredWidth: 200
            Layout.preferredHeight: 200
            Layout.alignment: Qt.AlignHCenter

            source: rootID.tour.imageUrl

        }

        Label {
            Layout.fillWidth: true

            text: rootID.tour.title
            font.bold: true
            font.pixelSize: 20
        }

        Label {
            Layout.fillWidth: true

            text: rootID.tour.description
            wrapMode: Text.WordWrap
        }

        TourInfoPanel {
            Layout.fillWidth: true
            Layout.preferredHeight: 100

            mode: GuiController.TourInfoMode.Info

            stopCount: rootID.tour.stopCount
            distance: rootID.tour.distance
        }

        StyledButton {
            Layout.fillWidth: true
            Layout.preferredHeight: 50

            text: qsTr("Start tour")
            onClicked: {
                TourController.currentTour = rootID.tour
                mainWindowID.toMainPage()
                GuiController.ChangeMapMode(GuiController.MapMode.DisplayTour)
            }
        }
    }
}