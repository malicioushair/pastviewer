import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "../Helpers/colors.js" as Colors

Dialog {
    id: errorDialogID

    modal: true
    focus: true
    width: Math.min(parent.width * 0.9, 400)
    padding: 2
    spacing: 0

    property string errorMessage: ""

    standardButtons: Dialog.Ok

    background: Rectangle {
        color: Colors.palette.bg
        radius: 8
        border.color: Colors.palette.border
        border.width: 2
    }

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true

            implicitHeight: headingID.implicitHeight + 36
            color: Colors.palette.toolbar
            radius: 6

            Rectangle {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: parent.radius
                color: parent.color
            }

            RowLayout {
                id: headingID

                anchors {
                    left: parent.left
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    margins: 24
                }
                spacing: 12

                Rectangle {
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    color: Colors.palette.error
                    radius: 17

                    Text {
                        anchors.centerIn: parent
                        text: "\u2715"
                        font.pixelSize: 20
                        color: "white"
                    }
                }

                Label {
                    Layout.fillWidth: true

                    text: qsTr("Error")
                    color: Colors.palette.text
                    font.pixelSize: 22
                    font.bold: true
                    wrapMode: Text.WordWrap
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.margins: 24

            text: errorDialogID.errorMessage
            color: Colors.palette.text
            font.pixelSize: 16
            wrapMode: Text.WordWrap
        }
    }

    footer: DialogButtonBox {
        padding: 24
        topPadding: 0
        alignment: Qt.AlignRight
        standardButtons: errorDialogID.standardButtons
        background: Item {}
        delegate: StyledButton {}
        onAccepted: errorDialogID.accept()
        onRejected: errorDialogID.reject()
    }
}
