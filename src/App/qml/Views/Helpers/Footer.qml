import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../../Helpers/colors.js" as Colors

ToolBar {
    id: rootID

    required property string text

    background: Rectangle {
        color: Colors.palette.toolbar
    }
    RowLayout {
        anchors.fill: parent
        Label {
            Layout.leftMargin: 10
            text: rootID.text
            color: Colors.palette.text
            font {
                bold: true
                pixelSize: 16
            }
            wrapMode: Text.WordWrap
        }
        Item {
            Layout.fillWidth: true
        }
    }
}