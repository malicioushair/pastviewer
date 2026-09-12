
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import "../GuiItems"
import "Helpers"

import PastViewer
import TourController

BasePage {
    id: rootID

    title: qsTr("Add tour stop")

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

            Label {
                font {
                    bold: true
                    pixelSize: 14
                }
                text: qsTr("Description")
            }

            Label {
                id: titleID

                text: qsTr("Title")
            }
            StyledTextArea {
                id: titleTextAreaID

                Layout.fillWidth: true
                Layout.preferredHeight: 40
            }

            Label {
                id: descriptionID

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
                }
                onRejected: {
                    console.log("Canceled")
                }
            }

            StyledButton {
                id: addImageButtonID

                text: qsTr("Add image")
                onClicked: addImageDialogID.open()
            }

            FileDialog {
                id: addAudioDialogID

                title: qsTr("Please add an audio")
                currentFolder: StandardPaths.standardLocations(StandardPaths.MusicLocation)
                onAccepted: {
                    console.log("Selected file: " + addAudioDialogID.selectedFile)
                }
                onRejected: {
                    console.log("Canceled")
                }
            }

            StyledButton {
                id: addAudioButtonID

                text: qsTr("Add audio")
                onClicked: addAudioDialogID.open()
            }

            RowLayout {
                StyledButton {
                    text: qsTr("Create")
                    onClicked: {
                        TourController.CreateTourStop(titleTextAreaID.text, descriptionTextAreaID.text, addImageDialogID.selectedFile, addAudioDialogID.selectedFile)
                        rootID.StackView.view.pop()
                    }
                }
                StyledButton {
                    text: qsTr("Cancel")
                    onClicked: rootID.StackView.view.pop()
                }
            }
        }
    }
}
