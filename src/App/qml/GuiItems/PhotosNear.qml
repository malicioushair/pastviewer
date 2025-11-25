import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import "../Helpers/colors.js" as Colors
import "../Helpers/utils.js" as Utils

Rectangle {
    id: rootID

    Layout.fillWidth: true
    Layout.preferredHeight: 210

    radius: 16
    color: Colors.palette.bg

    Rectangle {
        id: imagesNearbyID

        anchors {
            top: parent.top
            right: parent.right
            margins: 12
        }

        width: 24
        height: 24
        z: 999

        radius: 10

        color: Colors.palette.accentAlt
        border.color: Colors.palette.border

        Text {
            anchors.centerIn: parent
            text: listViewID.model.count
            color: "white"
        }
    }

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


            model: pastVuModelController.historyNearModel
            orientation: ListView.Horizontal
            spacing: 10

            onCountChanged: Utils.setTimeout(positionViewAtEnd, 300) // @TODO: add animation

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