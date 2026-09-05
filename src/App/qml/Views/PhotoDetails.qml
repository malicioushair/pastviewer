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
    required property string thumbnailSource
    required property int year

    header: Header {}

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

                    anchors.fill: parent
                    anchors.centerIn: parent

                    source: photoDetailsPageID.imageSource
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    antialiasing: true

                    visible: status == Image.Ready
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

        Button {
            id: cameraCtaID

            Layout.fillWidth: true
            Layout.topMargin: 10

            text: "📸 " + qsTr("Recreate this view")

            background: Rectangle {
                radius: 10
                color: cameraCtaID.pressed ? Colors.palette.accentAlt : Colors.palette.accent
                border.color: Colors.palette.border
                border.width: 2
                Behavior on color {
                    ColorAnimation {
                        duration: 150
                    }
                }
            }

            contentItem: Text {
                text: cameraCtaID.text
                color: "white"
                font.pixelSize: 16
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            padding: 12
            implicitHeight: 48

            onClicked: photoDetailsPageID.StackView.view.push("CameraMode.qml", {
                imageSource: imageSource,
                title: title,
                year: year
            })
        }
    }

    // Lightweight, one-time hint for zoom/pan and the recreate CTA
    OnboardingOverlay {
        id: photoDetailsOnboardingID

        anchors.fill: parent

        completionKey: "PhotoDetailsIntro"
        active: !GuiController.IsOnboardingStepCompleted(completionKey)

        steps: [
            {
                "title": qsTr("Explore the photo"),
                "body": qsTr("Pinch to zoom and drag to pan the historical photo. Use it to study the details of the past scene.")
            },
            {
                "title": qsTr("Recreate this view"),
                "body": qsTr("When you are ready, tap “Recreate this view” to open the camera and line up today’s scene with this photo.")
            }
        ]
    }
}
