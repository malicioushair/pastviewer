import QtQuick
import QtQuick.Controls

import "../Helpers/colors.js" as Colors

Page {
    id: rootID

    property bool blocksTipsPrompt: false

    function goBack() {
        if (rootID.StackView.view.depth > 1) {
            rootID.StackView.view.pop()
        }
    }

    background: Rectangle {
        color: Colors.palette.bg
    }

    Keys.onBackPressed: (event) => {
        // Android Back is handled by the window close handler. Leave Back
        // events available to the platform when the stack is at its root.
        event.accepted = true
                && Qt.platform.os !== "android"
                && rootID.StackView.view.depth > 1
        if (event.accepted)
            goBack()
    }
}
