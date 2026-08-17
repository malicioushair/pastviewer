import QtQuick
import QtQuick.Layouts

import "../GuiItems"
import "colors.js" as Colors

PromptDialog {
    id: rootID

    required property string version

    readonly property var changes: [
        {
            "title": qsTr("Nearby photo alerts"),
            "body": qsTr("Get a notification when you walk close to a historical photo.")
        },
        {
            "title": qsTr("Distance control"),
            "body": qsTr("Choose how close you need to be in Settings.")
        },
        {
            "title": qsTr("Works in the background"),
            "body": qsTr("Keep discovering while PastViewer is in the background, then tap an alert to open the matching photo.")
        }
    ]

    contentItem: ColumnLayout {
        spacing: 16

        Text {
            Layout.fillWidth: true

            text: qsTr("What's new in PastViewer %1").arg(rootID.version)
            wrapMode: Text.WordWrap
            color: Colors.palette.text
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            Layout.fillWidth: true

            text: qsTr("PastViewer can now help you discover history while you explore.")
            wrapMode: Text.WordWrap
            color: Colors.palette.text
            font.pixelSize: 14
        }

        Repeater {
            model: rootID.changes

            delegate: RowLayout {
                id: changeItemID

                required property var modelData

                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: 6

                    implicitWidth: 8
                    implicitHeight: 8
                    radius: 4
                    color: Colors.palette.accent
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        Layout.fillWidth: true

                        text: changeItemID.modelData.title
                        wrapMode: Text.WordWrap
                        color: Colors.palette.text
                        font.pixelSize: 14
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true

                        text: changeItemID.modelData.body
                        wrapMode: Text.WordWrap
                        color: Colors.palette.text
                        font.pixelSize: 14
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Item {
                Layout.fillWidth: true
            }

            StyledButton {
                text: qsTr("Got it")
                onClicked: rootID.accept()
            }
        }
    }
}
