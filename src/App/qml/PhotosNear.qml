import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    Layout.fillWidth: true
    Layout.preferredHeight: 210

    radius: 16
    color: Colors.palette.bg

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }
        spacing: 10

        Text {
            text: qsTr("History near you")
            font.bold: true
            font.pixelSize: 18
        }

        ListView {
            id: listViewID

            Layout.fillWidth: true
            Layout.fillHeight: true


            model: pastVuModelController.GetModel()
            orientation: ListView.Horizontal
            spacing: 10
            delegate: Item {
                width: 100
                height: 100

                Rectangle {
                    id: maskID

                    anchors.fill: parent
                    radius: 10
                    color: "white"
                    antialiasing: true
                }

                ShaderEffectSource {
                    id: maskTextureID

                    sourceItem: maskID
                }

                ColumnLayout {
                    anchors.fill: parent

                    Image {
                        id: imageID

                        Layout.fillWidth: true
                        Layout.preferredHeight: 100

                        source: Thumbnail
                        fillMode: Image.PreserveAspectCrop

                        layer.enabled: true
                        layer.effect: MultiEffect {
                                maskEnabled: true
                                maskSource: maskTextureID
                        }

                        TapHandler {
                            onTapped: Selected = true
                            onDoubleTapped: stackViewID.openPhotoDetails(Photo, Title, Year)
                        }
                    }

                    Text {
                        Layout.fillWidth: true

                        text: Title
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        text: Year
                    }
                }
            }
        }
    }
}