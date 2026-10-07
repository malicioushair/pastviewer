
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
                onAccepted: print("Added:", addImageDialogID.selectedFile) //TourController.UploadAsset(addImageDialogID.selectedFile)
                onRejected: {
                    console.log("Canceled")
                }
            }

            Connections {
                target: TourController

                onAssetUploadFinished: {
                    uploadImageIndicatorID.visible = true
                }
            }

            StyledButton {
                id: addImageButtonID

                text: qsTr("Add image")
                onClicked: addImageDialogID.open()
            }

            RowLayout {
                StyledButton {
                    id: uploadImageButtonID

                    text: qsTr("Upload Image")
                    onClicked: TourController.UploadAsset(addImageDialogID.selectedFile)
                }
                Rectangle {
                    id: uploadImageIndicatorID

                    Layout.preferredHeight: 10
                    Layout.preferredWidth: 10

                    radius: width
                    color: "green"
                    visible: false
                }
            }

            FileDialog {
                id: addAudioDialogID

                title: qsTr("Please add an audio")
                currentFolder: StandardPaths.standardLocations(StandardPaths.MusicLocation)
                onAccepted: TourController.UploadAsset(addAudioDialogID.selectedFile)
                onRejected: {
                    console.log("Canceled")
                }
            }

            Connections {
                target: TourController

                onAssetUploadFinished: {
                    uploadAudioIndicatorID.visible = true
                }
            }

            StyledButton {
                id: addAudioButtonID

                text: qsTr("Add audio")
                onClicked: addAudioDialogID.open()
            }

            RowLayout {
                StyledButton {
                    id: uploadAudioButtonID

                    text: qsTr("Upload Audio")
                    onClicked: TourController.UploadAsset(addAudioDialogID.selectedFile) // @todo: handle audio / image upload indications
                }
                Rectangle {
                    id: uploadAudioIndicatorID

                    Layout.preferredHeight: 10
                    Layout.preferredWidth: 10

                    radius: width
                    color: "green"
                    visible: false
                }
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
