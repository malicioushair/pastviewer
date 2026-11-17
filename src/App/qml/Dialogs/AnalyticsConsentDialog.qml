import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Analytics 1.0

import "../GuiItems"
import "../Helpers/colors.js" as Colors

PromptDialog {
    id: rootID
    closePolicy: Popup.NoAutoClose

    contentItem: ColumnLayout {
        spacing: 16
        Text {
            Layout.fillWidth: true
            text: qsTr("Help improve PastViewer")
            wrapMode: Text.WordWrap
            color: Colors.palette.text
            font.pixelSize: 18
            font.bold: true
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("Analytics includes only the necessary anonymous data that helps us understand the way the app is used and make it better. You can change your choice in Preferences.")
            wrapMode: Text.WordWrap
            color: Colors.palette.text
        }
        Text {
            textFormat: Text.RichText
            linkColor: Colors.palette.accent
            text: '<a href="https://github.com/malicioushair/pastviewer/blob/master/resources/legalTexts/PrivacyPolicy.txt">' + qsTr("Privacy policy") + '</a>'
            onLinkActivated: mainWindowID.openPrivacyPolicy
        }
        RowLayout {
            Layout.fillWidth: true
            StyledButton {
                Layout.fillWidth: true
                text: qsTr("Don't allow")
                transparent: true
                onClicked: {
                    Analytics.consentGranted = false
                    rootID.close()
                }
            }
            StyledButton {
                Layout.fillWidth: true
                text: qsTr("Allow analytics")
                transparent: true
                onClicked: {
                    Analytics.consentGranted = true
                    Analytics.Track("app_opened")
                    rootID.close()
                }
            }
        }
    }
}
