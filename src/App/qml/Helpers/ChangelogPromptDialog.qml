import QtQuick
import QtQuick.Layouts

import "../GuiItems"
import "colors.js" as Colors

PromptDialog {
    id: rootID

    required property string version

    readonly property var changes: [
        {
            "title": qsTr("Walking tours"),
            "body": qsTr("Browse available tours and follow routes on the map.")
        },
        {
            "title": qsTr("Stories at each stop"),
            "body": qsTr("Explore photo galleries and listen to audio narration where available.")
        },
        {
            "title": qsTr("Smoother browsing"),
            "body": qsTr("See when photos are loading and use the back gesture on iOS or Android.")
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

            text: qsTr("Explore local history with walking tours in PastViewer.")
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
