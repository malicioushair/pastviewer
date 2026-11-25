import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Colors

ToolBar {
    id: rootID

    property Component secondaryButton
    property bool boldTitle: false
    property alias label: labelID

    background: Rectangle {
        implicitHeight: 50
        color: Colors.toolbar
    }
    RowLayout {
        anchors.fill: parent
        ToolButton {
            Accessible.name: "Back"

            implicitHeight: rootID.height
            implicitWidth: implicitHeight
            text: "←"
            font.pointSize: 20
            onClicked: rootID.parent.StackView.view.pop()
            background: Rectangle {
                color: Colors.accent
            }
        }
        Label {
            id: labelID

            Layout.fillWidth: true
            text: rootID.parent.title
            color: Colors.text
            wrapMode: Text.Wrap
        }

        Loader {
            sourceComponent: rootID.secondaryButton
        }
    }
}