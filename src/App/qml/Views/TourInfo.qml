import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "../Helpers"
import "Helpers"

import PastViewer
import PurchaseController
import TourController

BasePage {
    id: rootID

    required property var tour

    property bool isOwned: tour.purchaseInfo.paid !== true
    property bool purchasePending: PurchaseController.IsTourPurchasePending(tour.id)
    property bool purchaseSucceeded: false

    title: qsTr("Tour Info")

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
    }

    Connections {
        target: GuiController

        function onOnboardingReset() {
            tourInfoOnboardingID.currentIndex = 0
            tourInfoOnboardingID.active = !GuiController.IsOnboardingStepCompleted(tourInfoOnboardingID.completionKey)
        }
    }

    OnboardingOverlay {
        id: tourInfoOnboardingID

        anchors.fill: parent
        z: 1000

        completionKey: "TourInfoIntro"
        active: !GuiController.IsOnboardingStepCompleted(completionKey)
        topped: currentIndex === 1 || currentIndex === 2

        highlightSteps: [
            { target: tourInfoPanelID.controlsItem, stepIndex: 1 },
            { target: unlockTourButtonID.visible ? unlockTourButtonID : startTourButtonID, stepIndex: 2 },
        ]

        steps: [
            {
                "title": qsTr("Get to know the tour"),
                "body": qsTr("Look at the cover photo and read the description to discover what this tour is about.")
            },
            {
                "title": qsTr("Plan your walk"),
                "body": qsTr("Here you see distance and the number of stops the tour has.")
            },
            {
                "title": qsTr("Start exploring"),
                "body": unlockTourButtonID.visible
                    ? qsTr("Tap Unlock tour to purchase this tour.")
                    : qsTr("Tap Start tour to open the map with this tour's route and stops.")
            }
        ]
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
            Layout.alignment: Qt.AlignHCenter

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
            id: tourInfoPanelID

            Layout.fillWidth: true
            Layout.preferredHeight: 100

            mode: GuiController.TourInfoMode.Info

            stopCount: rootID.tour.stopCount
            distance: rootID.tour.distance
        }

        StyledButton {
            id: startTourButtonID

            Layout.fillWidth: true
            Layout.preferredHeight: 50

            text: qsTr("Start tour")
            visible: !unlockTourButtonID.visible
            onClicked: {
                TourController.currentTour = rootID.tour
                mainWindowID.toMainPage()
                GuiController.ChangeMapMode(GuiController.MapMode.DisplayTour)
            }
        }

        Connections {
            target: PurchaseController

            function onTourPurchaseSucceeded(tourId) {
                if (rootID.tour.id !== tourId)
                    return

                rootID.purchasePending = false
                rootID.purchaseSucceeded = true
                rootID.isOwned = true
            }

            function onTourPurchasePending(tourId) {
                if (rootID.tour.id !== tourId)
                    return

                rootID.purchasePending = true
            }

            function onTourPurchasePendingCleared(tourId) {
                if (rootID.tour.id !== tourId)
                    return

                rootID.purchasePending = false
            }

            function onTourOwned(tourId, owned) {
                if (rootID.tour.id !== tourId)
                    return

                if (!owned && rootID.purchaseSucceeded)
                    return

                rootID.isOwned = owned
                if (owned)
                    rootID.purchasePending = false
            }
        }

        Label {
            Layout.fillWidth: true

            visible: rootID.purchasePending && !rootID.isOwned
            text: qsTr("Purchase pending approval.")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        StyledButton {
            id: unlockTourButtonID

            Layout.fillWidth: true
            Layout.preferredHeight: 50

            visible: !rootID.isOwned
            enabled: !rootID.purchasePending && !PurchaseController.purchaseInProgress
            reversedColors: true
            text: qsTr("Unlock tour")
            contentItem: Item {
                implicitWidth: unlockTourLabelID.implicitWidth
                implicitHeight: Math.max(unlockTourLabelID.implicitHeight, unlockTourBusyID.implicitHeight)

                Text {
                    id: unlockTourLabelID

                    anchors.centerIn: parent
                    text: unlockTourButtonID.text
                    font {
                        pixelSize: 16
                        bold: true
                    }
                    color: "white"
                    visible: !PurchaseController.purchaseInProgress
                }

                StyledBusyIndicator {
                    id: unlockTourBusyID

                    anchors.centerIn: parent
                    implicitHeight: 28
                    width: 28
                    height: 28
                    visible: PurchaseController.purchaseInProgress
                    running: visible
                }
            }
            onClicked: {
                PurchaseController.RequestTourPurchaseFlow(rootID.tour)
            }
        }
    }

    Component.onCompleted: {
        rootID.purchasePending = PurchaseController.IsTourPurchasePending(rootID.tour.id)
        PurchaseController.CheckTourOwned(rootID.tour)
    }
}
