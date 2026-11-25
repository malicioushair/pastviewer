import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../GuiItems"
import "Helpers"

import Analytics
import Colors
import PurchaseController
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

            delegate: Rectangle {
                id: tourRowID

                readonly property var tour: model.TourObj
                property bool purchased: false
                property bool purchaseSucceeded: false

                width: ListView.view.width
                height: 60
                radius: 8
                color: Colors.toolbar

                Component.onCompleted: {
                    if (tour.purchaseInfo.paid === true)
                        PurchaseController.CheckTourOwned(tour)
                }

                Connections {
                    target: PurchaseController

                    function onTourOwned(tourId, owned) {
                        if (true
                            && tourRowID.tour.id === tourId
                            && tourRowID.tour.purchaseInfo.paid === true
                            && (owned || !tourRowID.purchaseSucceeded))
                            tourRowID.purchased = owned
                    }

                    function onTourPurchaseSucceeded(tourId) {
                        if (true
                            && tourRowID.tour.id === tourId
                            && tourRowID.tour.purchaseInfo.paid === true) {
                            tourRowID.purchaseSucceeded = true
                            tourRowID.purchased = true
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 8

                    Item {
                        Layout.preferredHeight: 40
                        Layout.preferredWidth: 40

                        Image {
                            id: imageID

                            anchors.fill: parent

                            source: model.ImageUrl
                            asynchronous: true
                            fillMode: Image.PreserveAspectCrop
                            visible: false
                        }

                        Rectangle {
                            id: maskID

                            anchors.fill: parent

                            radius: 8
                            layer.enabled: true
                            visible: false
                        }

                        MultiEffect {
                            anchors.fill: parent

                            source: imageID
                            maskEnabled: true
                            maskSource: maskID
                            maskThresholdMin: 0.5
                            maskSpreadAtMin: 1.0
                        }
                        StyledBusyIndicator {
                            anchors.fill: parent
                            running: imageID.status === Image.Loading
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true

                        spacing: 2

                        Label {
                            Layout.fillWidth: true

                            text: model.Title
                            color: Colors.text
                            elide: Text.ElideRight
                            font.bold: true
                        }

                        Rectangle {
                            implicitWidth: purchasedLabelID.implicitWidth + 14
                            implicitHeight: purchasedLabelID.implicitHeight + 4
                            radius: height / 2

                            color: "#526c35"
                            visible: true //tourRowID.purchased

                            Label {
                                id: purchasedLabelID
                                anchors.centerIn: parent

                                text: "✓ " + qsTr("available")
                                color: "white"
                                font.pixelSize: 11
                                font.bold: true
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredHeight: 30
                        Layout.preferredWidth: 30

                        radius: 8
                        color: Colors.tourLanguage

                        Label {
                            anchors.centerIn: parent

                            text: model.Lang
                            color: Colors.text
                        }
                    }
                }

                TapHandler {
                    grabPermissions: PointerHandler.TakeOverForbidden
                    onTapped: mainWindowID.openTourInfoView(tourRowID.tour)
                }
            }
        }
    }
}
