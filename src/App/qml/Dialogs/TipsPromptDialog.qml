import QtQuick
import QtQuick.Layouts

import "../GuiItems"
import Colors

import GuiController

PromptDialog {
    id: rootID

    property bool outcomeRecorded: false

    function openPrompt() {
        outcomeRecorded = false
        open()
    }

    function dismissPrompt() {
        if (!outcomeRecorded) {
            outcomeRecorded = true
            rootID.discarded()
        }
        close()
    }

    onOpened: GuiController.MarkTipsPromptShown()
    onClosed: {
        if (!outcomeRecorded) {
            outcomeRecorded = true
            rootID.discarded()
        }
    }

    contentItem: ColumnLayout {
        spacing: 16

        Text {
            Layout.fillWidth: true

            text: qsTr("Enjoying PastViewer?")
            wrapMode: Text.WordWrap
            color: Colors.text
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            Layout.fillWidth: true

            text: qsTr("If PastViewer helped you rediscover a place, you can leave a one-time tip to support its continued development. The app remains fully available either way.")
            wrapMode: Text.WordWrap
            color: Colors.text
            font.pixelSize: 14
        }

        RowLayout {
            Layout.fillWidth: true

            spacing: 8

            Item {
                Layout.fillWidth: true
            }

            StyledButton {
                analyticsId: "tips_prompt_accept"
                text: qsTr("Leave a tip")
                onClicked: {
                    rootID.outcomeRecorded = true
                    rootID.accept()
                }
            }

            StyledButton {
                analyticsId: "tips_prompt_dismiss"
                text: qsTr("Not now")
                onClicked: rootID.dismissPrompt()
            }
        }
    }
}
