import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import "../GuiItems"
import "Helpers"
import "../Helpers/colors.js" as Colors

import PastViewer
import TourController

BasePage {
    id: rootID

    title: qsTr("Tour Description")

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
        ScrollView {
            id: scrollViewID

            Layout.fillWidth: true

            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                anchors.fill: parent

                Label {
                    font {
                        bold: true
                        pixelSize: 14
                    }
                    text: qsTr("Title")
                }

                StyledTextArea {
                    id: titleTextAreaID

                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                }

                Label {
                    font {
                        bold: true
                        pixelSize: 14
                    }
                    text: qsTr("Description")
                }

                StyledTextArea {
                    id: descriptionTextAreaID

                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                }

                FileDialog {
                    id: addImageDialogID

                    title: qsTr("Please add an image")
                    currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)
                    onAccepted: {
                        console.log("Selected file: " + addImageDialogID.selectedFile)
                        tourImageID.source = addImageDialogID.selectedFile
                    }
                    onRejected: {
                        console.log("Canceled")
                    }
                }
            }
        }

        Image {
            id: tourImageID

            Layout.maximumHeight: 200
            Layout.maximumWidth: 200

            fillMode: Image.PreserveAspectCrop
        }

        StyledButton {
            id: addImageButtonID

            text: qsTr("Add title image")
            onClicked: addImageDialogID.open()
        }

        Connections {
            target: TourController

            onAssetUploadFinished: {
                uploadIndicator.visible = true
            }
        }

        RowLayout {
            StyledButton {
                id: uploadImageButtonID

                text: qsTr("Upload image")

                onClicked: TourController.UploadAsset(addImageDialogID.selectedFile)
            }
            Rectangle {
                id: uploadIndicator

                Layout.preferredHeight: 10
                Layout.preferredWidth: 10

                radius: width
                color: "green"
                visible: false
            }
        }

        Item { Layout.fillHeight: true }
        RowLayout {
            Layout.alignment: Qt.AlignBottom

            StyledButton {
                text: qsTr("Next")
                onClicked: {
                    TourController.CreateNewTour(titleTextAreaID.text, descriptionTextAreaID.text, tourImageID.source)
                    GuiController.ChangeMapMode(GuiController.MapMode.TourCreation)
                    mainWindowID.showMap()
                }
            }
            StyledButton {
                text: qsTr("Cancel")
                onClicked: rootID.StackView.view.pop()
            }
        }
    }
}