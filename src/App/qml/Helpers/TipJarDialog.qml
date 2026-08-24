import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "colors.js" as Colors

import PastViewer

PromptDialog {
    id: rootID

    closePolicy: GuiController.tipPurchaseInProgress
                 ? Popup.NoAutoClose
                 : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onOpened: GuiController.LoadTipProducts()

    Connections {
        target: GuiController

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
            color: Colors.palette.text
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            Layout.fillWidth: true

            text: qsTranslate("TipsPromptDialog", "If PastViewer helped you rediscover a place, you can leave a one-time tip to support its continued development. The app remains fully available either way.")
            wrapMode: Text.WordWrap
            color: Colors.palette.text
            font.pixelSize: 14
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter

            visible: GuiController.tipProductsLoading
            running: visible
        }

        ColumnLayout {
            Layout.fillWidth: true

            visible: !GuiController.tipProductsLoading
                     && GuiController.tipProducts.length > 0
            spacing: 8

            Repeater {
                model: GuiController.tipProducts

                StyledButton {
                    required property var modelData

                    Layout.fillWidth: true

                    text: (modelData.title ? modelData.title + " · " : "")
                          + modelData.displayPrice
                    enabled: !GuiController.tipPurchaseInProgress
                    opacity: enabled ? 1 : 0.6
                    onClicked: GuiController.PurchaseTip(modelData.id)
                }
            }
        }

        StyledButton {
            Layout.alignment: Qt.AlignRight

            text: qsTranslate("TipsPromptDialog", "Not now")
            enabled: !GuiController.tipPurchaseInProgress
            opacity: enabled ? 1 : 0.6
            onClicked: rootID.close()
        }
    }
}
