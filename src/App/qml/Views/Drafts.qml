import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "Helpers"

import PastViewer
import TourController

BasePage {
    id: toursPageID

    title: qsTr("Drafts")

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

            model: TourController.GetDraftsModel()
            spacing: 8
            delegate: RowLayout {
                width: ListView.view.width
                StyledButton {
                    Layout.fillWidth: true

                    height: 40
                    text: model.Title
                }

                StyledButton {
                    width: 40
                    height: 40
                    text: qsTr("Publish")
                    onClicked: {
                        TourController.PublishTour(index)
                    }
                }

                StyledButton {
                    width: 40
                    height: 40
                    text: qsTr("del")
                    onClicked: model.Delete = true
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout. preferredHeight: 50

            color: "red"
        }
    }
}