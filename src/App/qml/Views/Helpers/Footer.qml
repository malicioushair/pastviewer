import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import GuiController

import Colors

ToolBar {
    id: rootID

    required property string text
    property color backgroundColor: Colors.toolbar

    background: Rectangle {
        color: rootID.backgroundColor
    }

    Connections {
        target: GuiController
        function onIsAdminChanged(value) {
            rootID.backgroundColor = value ? Colors.accentAlt : Colors.toolbar
        }
    }

    RowLayout {
        anchors.fill: parent
        Label {
            Layout.leftMargin: 10
            text: rootID.text
            color: Colors.text
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