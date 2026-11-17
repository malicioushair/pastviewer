import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import GuiController
import I18nController

import "../GuiItems"
import "../Helpers/colors.js" as Colors

Dialog {
    id: rootID
    modal: true
    focus: true
    width: Math.min(parent.width * 0.94, 720)
    height: parent.height * 0.9
    padding: 20

    background: Rectangle {
        color: Colors.palette.toolbar
        radius: 16
        border.color: Colors.palette.border
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: qsTr("Privacy policy")
            color: Colors.palette.text
            font.pixelSize: 18
            font.bold: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            TextArea {
                id: policyText
                objectName: "privacyPolicyText"
                text: {
                    const policy = GuiController.GetPrivacyPolicy(I18nController.currentLanguage)
                    return policy.length > 0 ? policy : qsTr("Unable to load the privacy policy.")
                }
                readOnly: true
                selectByMouse: true
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                color: Colors.palette.text
                background: null
            }
        }

        StyledButton {
            Layout.alignment: Qt.AlignRight
            text: qsTr("Close")
            transparent: true
            onClicked: rootID.close()
        }
    }

}
