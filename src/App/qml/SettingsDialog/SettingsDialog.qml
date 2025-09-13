import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Controls.Material

Dialog {
    id: settingsDialogID

    property string customSavePath: guiController.savePath

    anchors.centerIn: parent

    width: 600
    height: 200

    title: qsTr("Settings")
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

    ColumnLayout {
        id: settingsDialogLayoutID

        anchors.fill: parent

        spacing: 16

        SettingsDialogItem {
            id: savePathItemID

            label.text: qsTr("Files saved to: ") + guiController.savePath
            button {
                text: "📂"
                onClicked: folderDialogID.open()
            }

            FolderDialog {
                id: folderDialogID

                currentFolder: guiController.savePath
                onAccepted: guiController.savePath = currentFolder
            }
        }

    }
    onAccepted: guiController.savePath = folderDialogID.currentFolder
}