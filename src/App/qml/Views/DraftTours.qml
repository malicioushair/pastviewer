import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore

import "../Helpers/colors.js" as Colors

import "../GuiItems"
import "Helpers"

import PastViewer
import TourController

BasePage {
    id: toursPageID

    title: qsTr("Drafts")

    FileDialog {
        id: backupDialogID

        property var draftId

        title: qsTr("Backup")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("JSON files (*.json)")]
        defaultSuffix: "json"
        currentFolder: {
            const downloads = StandardPaths.standardLocations(StandardPaths.DownloadLocation)
            return downloads.length > 0 ? downloads[0]
                : StandardPaths.standardLocations(StandardPaths.DocumentsLocation)[0]
        }
        onAccepted: TourController.BackupDraft(draftId, selectedFile)
    }

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
    }

    ColumnLayout {
        anchors.fill: parent

        ListView {
            Layout.fillHeight: true
            Layout.fillWidth: true

            model: TourController.GetDraftsModel()
            spacing: 8
            delegate: ColumnLayout {
                width: ListView.view.width
                spacing: 4

                StyledButton {
                    id: draftTitleButtonID

                    Layout.fillWidth: true
                    Layout.preferredHeight: 40

                    text: model.Title
                    enabled: !model.Uploading
                    onClicked: mainWindowID.openDraftEditor(model.Id)

                    contentItem: Label {
                        text: draftTitleButtonID.text
                        color: draftTitleButtonID.enabled ? "white" : Colors.palette.disabledBtnText
                        font.pixelSize: 16
                        font.bold: true
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    StyledButton {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 40

                        Accessible.name: qsTr("Backup %1").arg(model.Title)

                        text: qsTr("Backup")
                        onClicked: {
                            backupDialogID.draftId = model.Id
                            const name = model.Title.replace(/[\\/:*?"<>|]/g, "_") + "_draft.json"
                            backupDialogID.selectedFile = backupDialogID.currentFolder + "/" + encodeURIComponent(name)
                            backupDialogID.open()
                        }
                    }

                    StyledButton {
                        id: publishButtonID

                        Layout.fillWidth: true
                        Layout.preferredHeight: 40

                        implicitWidth: publishTextID.implicitWidth + leftPadding + rightPadding + 26
                        text: qsTr("Publish")
                        enabled: !model.Uploading
                        onClicked: model.Publish = true

                        contentItem: RowLayout {
                            spacing: 6

                            StyledBusyIndicator {
                                Layout.preferredWidth: 20
                                Layout.preferredHeight: 20

                                padding: 0
                                running: model.Uploading
                                visible: running
                            }

                            Label {
                                id: publishTextID

                                Layout.fillWidth: true

                                text: publishButtonID.text
                                font.pixelSize: 16
                                font.bold: true
                                color: publishButtonID.enabled ? "white" : Colors.palette.disabledBtnText
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    StyledButton {
                        Layout.preferredHeight: 40

                        text: qsTr("del")
                        enabled: !model.Uploading
                        onClicked: model.Delete = true
                    }
                }
            }
        }

    }
}
