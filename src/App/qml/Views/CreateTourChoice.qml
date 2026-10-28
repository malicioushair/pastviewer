import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "Helpers"

BasePage {
    id: rootID

    title: qsTr("Create tour")

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 20
        }
        spacing: 24

        Label {
            Layout.fillWidth: true

            text: qsTr("How do you want to create the path?")
            wrapMode: Text.WordWrap
            font.pixelSize: 16
            font.bold: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledButton {
                Layout.fillWidth: true

                text: qsTr("Record by walking")
                onClicked: mainWindowID.openTourDescription(false)
            }

            Label {
                Layout.fillWidth: true

                text: qsTr("Walk the route and record the GPS path.")
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledButton {
                Layout.fillWidth: true

                text: qsTr("Place the path on the map")
                onClicked: mainWindowID.openTourDescription(true)
            }

            Label {
                Layout.fillWidth: true

                text: qsTr("Pan the map and add points to draw the path.")
                wrapMode: Text.WordWrap
                font.pixelSize: 14
            }
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
