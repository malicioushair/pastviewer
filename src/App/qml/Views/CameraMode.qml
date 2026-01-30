import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtCore

import "../Helpers/colors.js" as Colors
import "../Helpers/utils.js" as Utils
import "Helpers"

BasePage {
    id: rootID

    required property string imageSource
    required property int year

    function triggerShutterEffect() {
        shutterAnimationID.restart()
    }

    CaptureSession {
        id: captureSessionID

        camera: Camera {
            id: cameraID
            active: true
        }

        imageCapture: ImageCapture {
            id: imageCaptureID

            onImageSaved: (id, path) => {
                console.log("Image saved to:", path)
            }

            onImageCaptured: (id, preview) => {
                console.log("Image captured:", id)
            }
        }

        videoOutput: videoOutputID
    }

    header: Header {
        label.text: qsTr("Back")
    }

    footer: Footer {
        text: qsTr("Camera Mode")
    }

    ColumnLayout {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 10
        }

        spacing: 0

        Text {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: rootID.title
            font {
                bold: true
                pixelSize: 16
            }
        }

        Image {
            id: imageID

            Layout.fillWidth: true
            Layout.maximumHeight: 250

            source: rootID.imageSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            antialiasing: true
        }

        Text {
            Layout.fillWidth: true

            text: rootID.year
            horizontalAlignment: Text.AlignRight
        }

        Rectangle {
            id: videoContainerID

            Layout.fillWidth: true
            Layout.preferredHeight: 200
            Layout.topMargin: 15

            color: "black"
            clip: true

            VideoOutput {
                id: videoOutputID

                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectCrop
            }
        }

        Text {
            Layout.fillWidth: true

            text: qsTr("Now")
            horizontalAlignment: Text.AlignRight
        }

        // Camera shutter flash effect
        Rectangle {
            id: shutterFlashID
            anchors.fill: parent
            color: "white"
            opacity: 0
            visible: opacity > 0
            z: 1000

            SequentialAnimation {
                id: shutterAnimationID
                running: false

                NumberAnimation {
                    target: shutterFlashID
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 50
                }
                NumberAnimation {
                    target: shutterFlashID
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 150
                }
            }
        }
    }
    Rectangle {
        id: captureButtonID

        anchors {
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
            bottomMargin: 20
        }

        width: 72
        height: 72
        radius: width / 2
        color: "transparent"
        border.color: "white"
        border.width: 4

        Rectangle {
            anchors.centerIn: parent
            width: parent.width - 12
            height: parent.height - 12
            radius: width / 2
            color: "white"
            opacity: 0.5
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                rootID.triggerShutterEffect()

                // Take screenshot of the app window
                // Access the ApplicationWindow through the StackView's parent hierarchy
                let window = rootID.StackView.view
                while (window && window.parent) {
                    window = window.parent
                }

                captureButtonID.visible = false
                if (!window) {
                    console.error("Could not find window to capture")
                } else {
                    window.grabToImage((result) => {
                        guiController.SaveImage(result)
                    })
                }

                imageCaptureID.capture()
                // Make the capture button visible after the image has been grabbed
                Utils.setTimeout(() => { captureButtonID.visible = true }, 1)
            }
        }
    }
}
