import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../GuiItems"
import "Helpers"

import PastViewer
import TourController

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
                        mainWindowID.openCreateTour()
                    }
                }
            }

            SettingWithHint {
                description: qsTr("Browse your drafts")

                StyledButton {
                    text: qsTr("My draft tours")
                    onClicked: {
                        TourController.UpdateModel()
                        mainWindowID.openDrafts()
                    }
                }
            }
        }
    }
}