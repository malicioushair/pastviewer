import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtLocation
import QtPositioning

import PastViewer 1.0

import "../Helpers/colors.js" as Colors
import "../Helpers/utils.js" as Utils

Rectangle {
    id: rootID

    property Map map: null

    Layout.fillWidth: true
    Layout.preferredHeight: 210

    radius: 16
    color: Colors.palette.bg

    Connections {
        target: pastVuModelController
        enabled: true

        function onLoadingItems() {
            busyIndicatorID.visible = true
            imagesNearbyTextID.visible = false
        }

        function onItemsLoaded() {
            busyIndicatorID.visible = false
            imagesNearbyTextID.visible = true
        }
    }

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
            id: imagesNearbyTextID

            anchors.centerIn: parent
            text: listViewID.model.count
            color: "white"
        }

        BusyIndicator {
            id: busyIndicatorID

            anchors.centerIn: parent
            running: visible

            contentItem: Item {
                implicitWidth: 16
                implicitHeight: 16

                Item {
                    id: item
                    x: (parent.width - width) / 2
                    y: (parent.height - height) / 2

                    RotationAnimator {
                        target: item
                        running: busyIndicatorID.visible && busyIndicatorID.running
                        from: 0
                        to: 360
                        loops: Animation.Infinite
                        duration: 1250
                    }

                    Repeater {
                        id: repeater
                        model: 6

                        Rectangle {
                            x: item.width / 2 - width / 2
                            y: item.height / 2 - height / 2
                            width: 2
                            height: 2
                            radius: 1
                            color: "white"
                            transform: [
                                Translate {
                                    y: -6
                                },
                                Rotation {
                                    angle: index / repeater.count * 360
                                    origin.x: 1
                                    origin.y: 1
                                }
                            ]
                        }
                    }
                }
            }
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


            model: pastVuModelController.GetModel(ModelType.Raw)
            orientation: ListView.Horizontal
            spacing: 10

            onCountChanged: Utils.setTimeout(positionViewAtEnd, 300)

            delegate: Item {
                width: 100
                height: 160

                Rectangle {
                    id: maskID

                    anchors.fill: parent
                    anchors.margins: -3

                    radius: 10
                    border {
                        width: Selected ? 3 : 0
                        color: Selected ? Colors.palette.accent : "transparent"
                    }
                    color: Selected ? Colors.palette.shadowSoft : Colors.palette.bg
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

                        Layout.alignment: Qt.AlignTop
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
                            onTapped: {
                                model.Selected = true
                                if (model.IsClustered)
                                    mainWindowID.mapAnimationHelper.animateMapCenterAndZoom(
                                        model.Coordinate,
                                        model.ZoomToDecluster,
                                        model.Coordinate
                                    )
                                else
                                    mainWindowID.mapAnimationHelper.animateMapCenter(model.Coordinate)
                            }
                            onDoubleTapped: mainWindowID.openPhotoDetails(Photo, Title, Year)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 5

                        text: Title
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        Layout.leftMargin: 5
                        Layout.topMargin: -6
                        text: Year
                    }
                }
            }
        }
    }
}
