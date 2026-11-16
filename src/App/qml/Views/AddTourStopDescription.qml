
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtPositioning
import QtCore

import "../GuiItems"
import "Helpers"

import Analytics
import GuiController
import TourController

BasePage {
    Component.onCompleted: Analytics.Track("view_opened", { view: "add_tour_stop_description" })

    id: rootID

    property geoCoordinate stopCoordinate
    property bool placedStop: false

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

                singleLine: true

                Layout.fillWidth: true
                Layout.preferredHeight: 45
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

                title: qsTr("Please add images")
                fileMode: FileDialog.OpenFiles
                currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)
                onAccepted: print("Added:", addImageDialogID.selectedFiles)
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
                analyticsId: "tour_stop_choose_images"
                id: addImageButtonID

                text: qsTr("Add images")
                onClicked: addImageDialogID.open()
            }

            CompactImageGallery {
                Layout.fillWidth: true
                Layout.preferredHeight: implicitHeight
                imageUrls: addImageDialogID.selectedFiles
            }

            RowLayout {
                StyledButton {
                    analyticsId: "tour_stop_upload_images"
                    id: uploadImageButtonID

                    text: qsTr("Upload Images")
                    onClicked: TourController.UploadAssets(addImageDialogID.selectedFiles)
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
                onAccepted: TourController.UploadAssets(addAudioDialogID.selectedFiles)
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
                analyticsId: "tour_stop_choose_audio"
                id: addAudioButtonID

                text: qsTr("Add audio")
                onClicked: addAudioDialogID.open()
            }

            RowLayout {
                StyledButton {
                    analyticsId: "tour_stop_upload_audio"
                    id: uploadAudioButtonID

                    text: qsTr("Upload Audio")
                    onClicked: TourController.UploadAssets(addAudioDialogID.selectedFiles) // @todo: handle audio / image upload indications
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
                    analyticsId: "tour_stop_create"
                    text: qsTr("Create")
                    onClicked: {
                        const images = addImageDialogID.selectedFiles
                        if (rootID.placedStop)
                            TourController.CreateTourStop(titleTextAreaID.text, descriptionTextAreaID.text, images, addAudioDialogID.selectedFile, rootID.stopCoordinate)
                        else
                            TourController.CreateTourStop(titleTextAreaID.text, descriptionTextAreaID.text, images, addAudioDialogID.selectedFile)
                        rootID.StackView.view.pop()
                    }
                }
                StyledButton {
                    analyticsId: "tour_stop_cancel"
                    text: qsTr("Cancel")
                    onClicked: rootID.StackView.view.pop()
                }
            }
        }
    }
}
