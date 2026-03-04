import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

import "../Helpers/colors.js" as Colors
import "Helpers"

BasePage {
    id: photoDetailsPageID

    required property string imageSource
    required property string thumbnailSource
    required property int year

    header: Header {
        secondaryButton: ToolButton {
            text: "📸"
            onClicked: photoDetailsPageID.StackView.view.push("CameraMode.qml", {
                imageSource: imageSource,
                title: title,
                year: year
            })
            background: Rectangle {
                color: Colors.palette.accent
            }
        }
    }

    footer: Footer {
        text: qsTr("Year: ") + photoDetailsPageID.year
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }

        Item {
            id: viewportID

            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true

            Item {
                id: canvasID

                width: viewportID.width
                height: viewportID.height

                transformOrigin: Item.Center

                Image {
                    id: thumbnailImageID

                    anchors.fill: parent
                    anchors.centerIn: parent

                    source: photoDetailsPageID.thumbnailSource
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    antialiasing: true

                    visible: fullImageID.status !== Image.Ready
                }

                Image {
                    id: fullImageID

                    readonly property bool fitsViewPort: true
                            && fullImageID.status == Image.Ready
                            && fullImageID.paintedWidth * pinchHandlerID.scaleAxis.activeValue <= canvasID.width
                            && fullImageID.paintedHeight * pinchHandlerID.scaleAxis.activeValue <= canvasID.height

                    anchors.fill: parent
                    anchors.centerIn: parent

                    source: photoDetailsPageID.imageSource
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    antialiasing: true

                    visible: status == Image.Ready
                }

                PinchHandler {
                    id: pinchHandlerID

                    target: null

                    rotationAxis.enabled: false
                    scaleAxis.minimum: 1.0
                    xAxis.enabled: false
                    yAxis.enabled: false
                    dragThreshold: 40

                    onScaleChanged: (delta) => {
                        canvasID.scale = canvasID.scale * delta < 1.0 ? 1.0 : canvasID.scale * delta
                    }

                    onActiveChanged: canvasID.anchors.centerIn = !active && fullImageID.fitsViewPort ? viewportID : undefined
                }
                DragHandler {
                    id: dragHandlerID

                    target: null

                    xAxis.onActiveValueChanged: (delta) => {
                        if (!fullImageID.fitsViewPort)
                            canvasID.x += delta
                    }
                    yAxis.onActiveValueChanged: (delta) => {
                        if (!fullImageID.fitsViewPort)
                            canvasID.y += delta
                    }
                }
            }
        }
    }
}