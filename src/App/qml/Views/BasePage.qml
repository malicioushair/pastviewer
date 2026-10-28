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

    Keys.onBackPressed: goBack()
}
