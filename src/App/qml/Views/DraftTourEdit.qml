import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import TourController

import "../GuiItems"
import "Helpers"
import "../Helpers/colors.js" as Colors

BasePage {
    id: rootID

    required property var record
    property bool editingStop: false
    property var imageFiles: {
        if (editingStop) {
            if (record.imageFiles !== undefined)
                return record.imageFiles
            return record.imageFile ? [record.imageFile] : []
        }
        return record.imageFile ? [record.imageFile] : []
    }
    property url coverFile: !editingStop && record.imageFile ? record.imageFile : ""
    property url audioFile: editingStop ? record.audioFile : ""
    property string errorMessage: ""

    title: editingStop ? qsTr("Edit stop") : qsTr("Edit draft")

    function save() {
        errorMessage = editingStop
            ? TourController.UpdateDraftStop(record.id, titleFieldID.text, descriptionFieldID.text, imageFiles, audioFile)
            : TourController.UpdateDraftDetails(record.id, titleFieldID.text, descriptionFieldID.text, coverFile)
        return errorMessage.length === 0
    }

    header: Header { boldTitle: true }

    ScrollView {
        id: scrollID

        implicitWidth: 0
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: scrollID.availableWidth
            spacing: 12

            Label { text: qsTr("Title") }
            TextField {
                id: titleFieldID
                Layout.fillWidth: true
                text: rootID.record.title
                color: Colors.palette.text
                background: Rectangle { color: Colors.palette.textEdit; radius: 8 }
            }

            Label { text: qsTr("Description") }
            TextArea {
                id: descriptionFieldID
                Layout.fillWidth: true
                Layout.minimumHeight: 120
                text: rootID.record.description
                wrapMode: TextEdit.Wrap
                color: Colors.palette.text
                background: Rectangle { color: Colors.palette.textEdit; radius: 8 }
            }

            Image {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                visible: !rootID.editingStop && rootID.coverFile.toString().length > 0
                source: rootID.coverFile
                fillMode: Image.PreserveAspectFit
            }
            Repeater {
                model: rootID.editingStop ? rootID.imageFiles : []
                Image {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180
                    source: modelData
                    fillMode: Image.PreserveAspectFit
                }
            }
            RowLayout {
                StyledButton {
                    text: rootID.editingStop ? qsTr("Choose images") : qsTr("Choose cover")
                    onClicked: coverDialogID.open()
                }
                StyledButton {
                    text: qsTr("Remove images")
                    visible: rootID.editingStop
                             ? rootID.imageFiles.length > 0
                             : rootID.coverFile.toString().length > 0
                    onClicked: {
                        if (rootID.editingStop)
                            rootID.imageFiles = []
                        else
                            rootID.coverFile = ""
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: rootID.editingStop && rootID.audioFile.toString().length > 0
                text: rootID.audioFile.toString().split('/').pop()
                wrapMode: Text.Wrap
            }
            RowLayout {
                visible: rootID.editingStop
                StyledButton {
                    text: qsTr("Choose audio")
                    onClicked: audioDialogID.open()
                }
                StyledButton {
                    text: qsTr("Remove audio")
                    visible: rootID.audioFile.toString().length > 0
                    onClicked: rootID.audioFile = ""
                }
            }

            Label {
                Layout.fillWidth: true
                visible: rootID.errorMessage.length > 0
                text: rootID.errorMessage
                color: Colors.palette.accent
                wrapMode: Text.Wrap
            }

            StyledButton {
                Layout.fillWidth: true
                visible: !rootID.editingStop
                text: qsTr("Save and edit route / stops")
                onClicked: {
                    if (rootID.save())
                        mainWindowID.openEditDraftMap(rootID.record.id)
                }
            }
            RowLayout {
                StyledButton {
                    text: qsTr("Save")
                    onClicked: {
                        if (rootID.save())
                            rootID.goBack()
                    }
                }
                StyledButton {
                    text: qsTr("Cancel")
                    onClicked: rootID.goBack()
                }
            }
        }
    }

    FileDialog {
        id: coverDialogID
        title: rootID.editingStop ? qsTr("Choose images") : qsTr("Choose image")
        fileMode: rootID.editingStop ? FileDialog.OpenFiles : FileDialog.OpenFile
        nameFilters: [qsTr("Images (*.png *.jpg *.jpeg *.webp *.heic)")]
        onAccepted: {
            if (rootID.editingStop)
                rootID.imageFiles = selectedFiles
            else
                rootID.coverFile = selectedFile
        }
    }
    FileDialog {
        id: audioDialogID
        title: qsTr("Choose audio")
        nameFilters: [qsTr("Audio (*.mp3 *.m4a *.wav *.ogg *.aac)")]
        onAccepted: rootID.audioFile = selectedFile
    }
}
