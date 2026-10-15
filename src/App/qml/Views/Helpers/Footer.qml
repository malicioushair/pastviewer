import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PastViewer

import "../../Helpers/colors.js" as Colors

ToolBar {
    id: rootID

    required property string text
    property color backgroundColor: Colors.palette.toolbar

    background: Rectangle {
        color: rootID.backgroundColor
    }

    Connections {
        target: GuiController
        function onIsAdminChanged(value) {
            rootID.backgroundColor = value ? Colors.palette.accentAlt : Colors.palette.toolbar
        }
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