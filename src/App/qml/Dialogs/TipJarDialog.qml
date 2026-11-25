import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import Colors

import GuiController
import PurchaseController

PromptDialog {
    id: rootID

    closePolicy: PurchaseController.purchaseInProgress
                 ? Popup.NoAutoClose
                 : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onOpened: PurchaseController.LoadTipProducts()

    Connections {
        target: PurchaseController

        function onTipPurchaseSucceeded() {
            rootID.close()
        }

        function onTipPurchasePending() {
            rootID.close()
        }

        function onTipOperationFailed() {
            rootID.close()
        }
    }

    contentItem: ColumnLayout {
        spacing: 16

        Text {
            Layout.fillWidth: true

            text: qsTranslate("TipsPromptDialog", "Enjoying PastViewer?")
            wrapMode: Text.WordWrap
            color: Colors.text
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            Layout.fillWidth: true

            text: qsTranslate("TipsPromptDialog", "If PastViewer helped you rediscover a place, you can leave a one-time tip to support its continued development. The app remains fully available either way.")
            wrapMode: Text.WordWrap
            color: Colors.text
            font.pixelSize: 14
        }

        StyledBusyIndicator {
            Layout.alignment: Qt.AlignHCenter

            visible: PurchaseController.tipProductsLoading
            running: visible
        }

        ColumnLayout {
            Layout.fillWidth: true

            visible: !PurchaseController.tipProductsLoading
                     && PurchaseController.tipProducts.length > 0
            spacing: 8

            Repeater {
                model: PurchaseController.tipProducts

                StyledButton {
                    analyticsId: "tip_purchase"
                    required property var modelData

                    Layout.fillWidth: true

                    text: (modelData.title ? modelData.title + " · " : "")
                          + modelData.displayPrice
                    enabled: !PurchaseController.purchaseInProgress
                    opacity: enabled ? 1 : 0.6
                    onClicked: PurchaseController.PurchaseTip(modelData.id)
                }
            }
        }

        StyledButton {
            analyticsId: "tip_dismiss"
            Layout.alignment: Qt.AlignRight

            text: qsTranslate("TipsPromptDialog", "Not now")
            enabled: !PurchaseController.purchaseInProgress
            opacity: enabled ? 1 : 0.6
            onClicked: rootID.close()
        }
    }
}
