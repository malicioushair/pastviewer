import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Material

Item {
    id: settingsItemID

    Layout.fillWidth: true
    Layout.fillHeight: true

    property string customSavePath: guiController.savePath
    property alias label: labelID
    property alias button: buttonID
    property alias checked: checkBoxID.checkedOpt

    RowLayout {
        id: itemLayoutID

        anchors.fill: parent

        spacing: 10

        Label {
            id: labelID
        }

        Item {
            id: horizontalSpacer

            Layout.fillWidth: true
            Layout.fillHeight: true

            visible: true
        }

        Button {
            id: buttonID

            Layout.alignment: Qt.AlignRight

            visible: text // no button if text is empty, undefined, or null
        }

        CheckBox {
            id: checkBoxID

            property var checkedOpt: undefined // !!!use this property for setting the checkbox, NOT the original checked!!!

            visible: typeof checkedOpt === "boolean"
            checked: (typeof checkedOpt === "boolean") ? checkedOpt : false

            onToggled: checkedOpt = checked
        }
    }
}