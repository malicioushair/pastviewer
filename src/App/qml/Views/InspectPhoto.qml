import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

import "../Helpers/colors.js" as Colors
import "../Helpers"
import "Helpers"

import PastViewer

BasePage {
    id: photoDetailsPageID

    blocksTipsPrompt: photoDetailsOnboardingID.active

    required property string imageSource

    title: qsTr("Stop Details")

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
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

            readonly property real paintedImageWidth: fullImageID.status === Image.Ready
                    ? fullImageID.paintedWidth : thumbnailImageID.paintedWidth
            readonly property real paintedImageHeight: fullImageID.status === Image.Ready
                    ? fullImageID.paintedHeight : thumbnailImageID.paintedHeight
            readonly property real horizontalPanLimit: Math.max(0, (paintedImageWidth * canvasID.scale - width) / 2)
            readonly property real verticalPanLimit: Math.max(0, (paintedImageHeight * canvasID.scale - height) / 2)

            function clampCanvasPosition() {
                canvasID.x = Math.max(-horizontalPanLimit,
                        Math.min(horizontalPanLimit, canvasID.x))
                canvasID.y = Math.max(-verticalPanLimit,
                        Math.min(verticalPanLimit, canvasID.y))
            }

            function scheduleCanvasClamp() {
                Qt.callLater(clampCanvasPosition)
            }

            onHorizontalPanLimitChanged: scheduleCanvasClamp()
            onVerticalPanLimitChanged: scheduleCanvasClamp()

            Item {
                id: canvasID

                width: viewportID.width
                height: viewportID.height

                transformOrigin: Item.Center

                Image {
                    id: fullImageID

                    anchors {
                        fill: parent
                        centerIn: parent
                    }

                    source: photoDetailsPageID.imageSource
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    antialiasing: true

                    visible: status == Image.Ready
                }

                StyledBusyIndicator {
                    anchors.centerIn: parent
                    implicitWidth: 80
                    implicitHeight: implicitWidth
                    visible: fullImageID.status !== Image.Ready
                }
            }

            PinchHandler {
                id: pinchHandlerID

                property real initialScale: 1.0
                property real initialX: 0.0
                property real initialY: 0.0
                property point initialCentroid: Qt.point(0, 0)

                function updateCanvasTransform() {
                    if (!active)
                        return

                    const newScale = Math.max(1.0, initialScale * activeScale)
                    const scaleRatio = newScale / initialScale
                    canvasID.scale = newScale

                    // Preserve the photo point under the initial pinch centroid
                    // while also following two-finger translation.
                    canvasID.x = scaleRatio * initialX + (1.0 - scaleRatio) * (initialCentroid.x - viewportID.width / 2) + activeTranslation.x
                    canvasID.y = scaleRatio * initialY + (1.0 - scaleRatio) * (initialCentroid.y - viewportID.height / 2) + activeTranslation.y
                    viewportID.clampCanvasPosition()
                }

                target: null

                rotationAxis.enabled: false
                scaleAxis.minimum: 1.0

                onActiveChanged: {
                    if (active) {
                        initialScale = canvasID.scale
                        initialX = canvasID.x
                        initialY = canvasID.y
                        initialCentroid = centroid.position
                    } else {
                        viewportID.clampCanvasPosition()
                    }
                }
                onScaleChanged: updateCanvasTransform()
                onTranslationChanged: updateCanvasTransform()
            }
            DragHandler {
                id: dragHandlerID

                target: null

                minimumPointCount: 1
                maximumPointCount: 1

                xAxis.onActiveValueChanged: (delta) => {
                    canvasID.x += delta
                    viewportID.clampCanvasPosition()
                }
                yAxis.onActiveValueChanged: (delta) => {
                    canvasID.y += delta
                    viewportID.clampCanvasPosition()
                }

                onActiveChanged: {
                    if (!active)
                        viewportID.clampCanvasPosition()
                }
            }
        }
    }
}
