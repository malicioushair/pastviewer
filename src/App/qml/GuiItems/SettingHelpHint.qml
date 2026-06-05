import QtQuick
import QtQuick.Controls

import "../Helpers/colors.js" as Colors

AbstractButton {
    id: controlID

    property string description: ""

    implicitWidth: 22
    implicitHeight: 22

    background: Rectangle {
        radius: width / 2
        color: controlID.pressed ? Colors.palette.accentAlt : Colors.palette.toolbar
        border {
            color: Colors.palette.border
            width: 1
        }
    }

    contentItem: Text {
        text: "?"
        font {
            pixelSize: 13
            bold: true
        }
        color: Colors.palette.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    onClicked: descriptionPopupID.visible ? descriptionPopupID.close() : descriptionPopupID.open()

    Popup {
        id: descriptionPopupID

        leftMargin: 8
        topMargin: 8
        width: parent ? Math.min(280, parent.width * 0.85) : 280

        parent: Overlay.overlay
        modal: true
        focus: true
        padding: 16
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        onOpened: y = controlID.mapToItem(parent, controlID.width, controlID.height).y + topMargin

        background: Rectangle {
            radius: 12
            color: Colors.palette.toolbar
            border {
                color: Colors.palette.border
                width: 1
            }
        }

        contentItem: Label {
            text: controlID.description
            wrapMode: Text.WordWrap
            width: descriptionPopupID.availableWidth
            color: Colors.palette.text
            font.pixelSize: 14
        }
    }
}
