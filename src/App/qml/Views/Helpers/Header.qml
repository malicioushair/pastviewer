import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../../Helpers/colors.js" as Colors

ToolBar {
    id: rootID

    property Component secondaryButton
    property bool boldTitle: false
    property alias label: labelID

    background: Rectangle {
        color: Colors.palette.toolbar
    }
    RowLayout {
        anchors.fill: parent
        ToolButton {
            text: "←"
            onClicked: rootID.parent.StackView.view.pop()
            background: Rectangle {
                color: Colors.palette.accent
            }
        }
        Label {
            id: labelID

            Layout.fillWidth: true
            text: rootID.parent.title
            color: Colors.palette.text
            wrapMode: Text.Wrap
        }

        Loader {
            sourceComponent: rootID.secondaryButton
        }
    }
}