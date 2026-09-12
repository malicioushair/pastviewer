import QtQuick
import QtQuick.Controls
import QtQuick.Layouts


import "../Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    property alias text: textAreaID.text

    color: Colors.palette.textEdit
    radius: 8

    TextArea {
        id: textAreaID

        property string acceptedText: ""
        property bool restoringText: false

        anchors {
            fill: parent
            margins: 5
        }
        clip: true
        wrapMode: TextEdit.Wrap
        onTextChanged: {
            if (restoringText)
                return

            if (contentHeight > height - topPadding - bottomPadding) {
                const previousCursor = cursorPosition

                restoringText = true
                text = acceptedText
                cursorPosition = Math.min(previousCursor, text.length)
                restoringText = false
            } else {
                acceptedText = text
            }
        }
    }
}