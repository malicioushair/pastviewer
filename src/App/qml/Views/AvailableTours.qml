import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "Helpers"
import "../Helpers/colors.js" as Colors

import TourController

BasePage {
    id: availableToursID

    title: qsTr("Available tours")

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ListView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.margins: 10

            model: TourController.GetAvailableToursModel()
            spacing: 10

            delegate: RowLayout {
                width: parent.width
                height: 40

                Rectangle {
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 40

                    color: "transparent"
                    Image {
                        id: imageID

                        anchors.fill: parent
                        source: model.ImageUrl
                    }
                    BusyIndicator {
                        anchors.fill: parent
                        running: imageID.status === Image.Loading
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    color: Colors.palette.toolbar
                    Label {
                        anchors.centerIn: parent
                        text: model.Title
                    }
                }
            }
        }
    }
}