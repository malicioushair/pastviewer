import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "Helpers"

import PastViewer

BasePage {
    id: toursPageID

    title: qsTr("Tours")

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
    }

    ScrollView {
        id: toursScrollViewID

        anchors {
            fill: parent
            margins: 20
        }

        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            anchors.fill: parent

            SettingWithHint {
                description: qsTr("View the list of available tours in the vicinity")

                StyledButton {
                    text: qsTr("Browse tours")
                    onClicked: print("Browse tours")
                }
            }

            SettingWithHint {
                description: qsTr("Create your own tour")

                StyledButton {
                    text: qsTr("Create tour")
                    onClicked: {
                        GuiController.ChangeMapMode(GuiController.MapMode.TourCreation)
                        mainWindowID.showMap()
                    }
                }
            }
        }
    }
}