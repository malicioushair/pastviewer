import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

import "../Helpers/colors.js" as Colors
import "Helpers"

BasePage {
    id: photoDetailsPageID

    required property string imageSource
    required property int year

    header: Header {
        secondaryButton: ToolButton {
            visible: guiController.IsDebug() // @TODO: enable, when feature is 100% ready
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
                    id: imageID

                    readonly property bool fitsViewPort: true
                            && imageID.status == Image.Ready
                            && imageID.paintedWidth * pinchHandlerID.scaleAxis.activeValue <= canvasID.width
                            && imageID.paintedHeight * pinchHandlerID.scaleAxis.activeValue <= canvasID.height

                    anchors.fill: parent
                    anchors.centerIn: parent

                    source: photoDetailsPageID.imageSource
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    antialiasing: true
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

                    onActiveChanged: canvasID.anchors.centerIn = !active && imageID.fitsViewPort ? viewportID : undefined
                }
                DragHandler {
                    id: dragHandlerID

                    target: null

                    xAxis.onActiveValueChanged: (delta) => {
                        if (!imageID.fitsViewPort)
                            canvasID.x += delta
                    }
                    yAxis.onActiveValueChanged: (delta) => {
                        if (!imageID.fitsViewPort)
                            canvasID.y += delta
                    }
                }
            }
        }
    }
}