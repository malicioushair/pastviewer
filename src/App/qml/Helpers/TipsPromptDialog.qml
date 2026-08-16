import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "colors.js" as Colors

Dialog {
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

    modal: true
    focus: true
    width: Math.min(parent.width * 0.9, 420)
    padding: 20
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onOpened: guiController.MarkTipsPromptShown()
    onClosed: {
        if (!outcomeRecorded) {
            outcomeRecorded = true
            rootID.discarded()
        }
    }

    background: Rectangle {
        radius: 16
        color: Colors.palette.toolbar
        border.color: Colors.palette.border
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 16

        Text {
            Layout.fillWidth: true

            text: qsTr("Enjoying PastViewer?")
            wrapMode: Text.WordWrap
            color: Colors.palette.text
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            Layout.fillWidth: true

            text: qsTr("If PastViewer helped you rediscover a place, you can leave a one-time tip to support its continued development. The app remains fully available either way.")
            wrapMode: Text.WordWrap
            color: Colors.palette.text
            font.pixelSize: 14
        }

        RowLayout {
            Layout.fillWidth: true

            spacing: 8

            Item {
                Layout.fillWidth: true
            }

            StyledButton {
                text: qsTr("Leave a tip")
                onClicked: {
                    rootID.outcomeRecorded = true
                    rootID.accept()
                }
            }

            StyledButton {
                text: qsTr("Not now")
                onClicked: rootID.dismissPrompt()
            }
        }
    }
}
