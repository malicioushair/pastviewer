import QtQuick
import QtQuick.Controls


import "../Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    property alias text: textAreaID.text
    property string inputObjectName: ""
    property string accessibleName: ""
    property bool singleLine: false

    color: Colors.palette.textEdit
    radius: 8
    border.width: singleLine ? (singleLineInputID.activeFocus ? 2 : 1) : 0
    border.color: !enabled ? Colors.palette.disabledBtnBorder
                          : singleLineInputID.activeFocus ? Colors.palette.accentAlt
                                                         : Colors.palette.border

    TextMetrics {
        id: singleLineMetricsID
        font: singleLineInputID.font
    }

    TextField {
        id: singleLineInputID

        Accessible.name: rootID.accessibleName
        objectName: rootID.singleLine ? rootID.inputObjectName : ""

        anchors {
            fill: parent
            margins: 5
        }
        visible: rootID.singleLine
        text: textAreaID.text
        padding: 6
        font.pixelSize: 16
        color: enabled ? Colors.palette.text : Colors.palette.disabledBtnText
        selectionColor: Colors.palette.accentAlt
        selectedTextColor: "white"
        verticalAlignment: TextInput.AlignVCenter
        background: null
        clip: true

        onTextEdited: {
            singleLineMetricsID.text = text
            const textWidth = Math.max(singleLineMetricsID.advanceWidth,
                                       singleLineMetricsID.boundingRect.width)
            if (textWidth <= width - leftPadding - rightPadding) {
                textAreaID.text = text
            } else {
                // Reject edits that would require horizontal scrolling.
                const previousCursor = Math.max(0, cursorPosition - (text.length - textAreaID.text.length))
                text = Qt.binding(function() { return textAreaID.text })
                cursorPosition = Math.min(previousCursor, text.length)
            }
        }
    }

    ScrollView {
        id: scrollViewID

        visible: !rootID.singleLine

        anchors {
            fill: parent
            margins: 5
        }
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        ScrollBar.vertical: ScrollBar {
            id: verticalScrollBarID

            parent: scrollViewID
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
            }
            orientation: Qt.Vertical
            size: Math.min(1.0, scrollViewID.availableHeight / Math.max(1, scrollViewID.contentHeight))
            policy: ScrollBar.AsNeeded
            visible: size < 1.0
            padding: 2
            implicitWidth: 8

            contentItem: Rectangle {
                implicitWidth: 4
                implicitHeight: 24
                radius: width / 2
                color: Colors.palette.text
                opacity: verticalScrollBarID.pressed ? 0.6
                         : verticalScrollBarID.hovered ? 0.45 : 0.25
            }
            background: Item {}
        }

        TextArea {
            id: textAreaID

            Accessible.name: rootID.accessibleName
            objectName: rootID.singleLine ? "" : rootID.inputObjectName

            wrapMode: TextEdit.Wrap
            rightPadding: 6 + scrollViewID.effectiveScrollBarWidth
        }
    }
}
