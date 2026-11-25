import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "Helpers"
import Colors

import Analytics
import TourController

BasePage {
    Component.onCompleted: Analytics.Track("view_opened", { view: "available_tours" })

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
                width: ListView.view.width
                height: 40

                Rectangle {
                    Layout.preferredHeight: 40
                    Layout.preferredWidth: 40

                    color: "transparent"
                    Image {
                        id: imageID

                        anchors.fill: parent
                        source: model.ImageUrl
                        asynchronous: true
                    }
                    StyledBusyIndicator {
                        anchors.fill: parent
                        running: imageID.status === Image.Loading
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    color: Colors.toolbar
                    Label {
                        anchors.centerIn: parent
                        text: model.Title
                    }
                    TapHandler {
                        grabPermissions: PointerHandler.TakeOverForbidden
                        onTapped: mainWindowID.openTourInfoView(model.TourObj)
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40

                    color: Colors.toolbar
                    Label {
                        anchors.centerIn: parent
                        text: qsTr(model.Lang)
                    }
                }
            }
        }
    }
}