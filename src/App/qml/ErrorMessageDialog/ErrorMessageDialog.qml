import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors

Dialog {
    id: errorDialogID

    modal: true
    width: Math.min(parent.width * 0.9, 400)

    property string errorMessage: ""

    standardButtons: Dialog.Ok

    onAccepted: Qt.quit()

    ColumnLayout {
        RowLayout {
            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
                Layout.alignment: Qt.AlignVCenter
                color: Colors.palette.error
                radius: 20

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 24
                    font.bold: true
                    color: "white"
                }
            }

            Text {
                text: qsTr("Critical error")
                color: "white"
                font.pixelSize: 24
            }
        }
        Label {
            Layout.margins: 20

            text: errorDialogID.errorMessage
            wrapMode: Text.WordWrap
            width: parent.width
        }
    }
}