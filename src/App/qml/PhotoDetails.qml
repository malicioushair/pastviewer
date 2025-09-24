import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: rootID

    required property string imageSource
    required property int year

    header: ToolBar {
        ToolButton {
            text: "←"
            onClicked: rootID.StackView.view.pop()
        }
        Label {
            anchors.centerIn: parent
            text: rootID.title
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }
        spacing: 10

        Image {
            Layout.fillWidth: true
            Layout.preferredHeight: width * 0.75

            source: rootID.imageSource
            fillMode: Image.PreserveAspectFit
        }

        Label {
            Layout.fillWidth: true

            text: rootID.title
            font {
                bold: true
                pixelSize: 16
            }
            wrapMode: Text.WordWrap
        }

        Label {
            Layout.fillWidth: true

            text: "Year: " + rootID.year
            font.pixelSize: 14
        }
    }
}