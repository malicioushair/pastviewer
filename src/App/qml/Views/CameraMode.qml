import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import QtCore

import "../Helpers/colors.js" as Colors
import "../Helpers/utils.js" as Utils
import "../Helpers"
import "Helpers"

BasePage {
    id: rootID

    required property string imageSource
    required property int year
    
    property string thumbnailSource: ""
    property bool thumbnailVisible: false
    property bool thumbnailAnimationVisible: false
    property bool controlsHidden: false
    property double thumbnailScale: 0.2

    function triggerShutterEffect() {
        shutterAnimationID.restart()
    }

    function animateThumbnail(savedImageUrl) {
        thumbnailSource = savedImageUrl
        thumbnailVisible = false
        thumbnailAnimationVisible = true
        thumbnailAnimationID.x = 0
        thumbnailAnimationID.y = 0
        thumbnailAnimationID.width = rootID.width
        thumbnailAnimationID.height = rootID.height
        thumbnailAnimationID.radius = 0
        thumbnailShrinkAnimationID.restart()
    }

    CaptureSession {
        id: captureSessionID

        camera: Camera {
            id: cameraID
            active: true
        }

        videoOutput: videoOutputID
    }

    header: Header {
        label.text: qsTr("Back")
    }

    footer: Footer {
        text: "PastViewer"
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

    Rectangle {
        id: thumbnailAnimationID

        visible: rootID.thumbnailAnimationVisible
        z: 999
        clip: true
        color: Colors.palette.toolbar
        border {
            color: Colors.palette.border
            width: visible ? 1 : 0
        }

        Image {
            anchors.fill: parent
            anchors.margins: parent.radius > 0 ? 3 : 0
            source: rootID.thumbnailSource
            fillMode: Image.PreserveAspectCrop
            asynchronous: false
            cache: false
        }
    }

    ParallelAnimation {
        id: thumbnailShrinkAnimationID

        NumberAnimation {
            target: thumbnailAnimationID
            property: "x"
            to: thumbnailButtonID.x
            duration: 450
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: thumbnailAnimationID
            property: "y"
            to: thumbnailButtonID.y
            duration: 450
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: thumbnailAnimationID
            property: "width"
            to: rootID.width * rootID.thumbnailScale
            duration: 450
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: thumbnailAnimationID
            property: "height"
            to: rootID.height * rootID.thumbnailScale
            duration: 450
            easing.type: Easing.OutCubic
        }

        NumberAnimation {
            target: thumbnailAnimationID
            property: "radius"
            to: thumbnailButtonID.radius
            duration: 450
            easing.type: Easing.OutCubic
        }

        onFinished: {
            rootID.controlsHidden = false
            rootID.thumbnailAnimationVisible = false
            rootID.thumbnailVisible = true
            Utils.setTimeout(() => { rootID.thumbnailVisible = false }, 3000)
        }
    }

    Rectangle {
        id: thumbnailButtonID

        anchors {
            right: parent.right
            bottom: parent.bottom
            margins: 20
        }

        width: rootID.width * rootID.thumbnailScale
        height: rootID.height * rootID.thumbnailScale
        radius: 10
        visible: rootID.thumbnailVisible && !rootID.controlsHidden
        clip: true
        color: Colors.palette.toolbar
        border {
            color: Colors.palette.border
            width: 1
        }

        Image {
            anchors.fill: parent
            anchors.margins: 3
            source: rootID.thumbnailSource
            fillMode: Image.PreserveAspectCrop
            asynchronous: false
            cache: false
        }

        Rectangle {
            id: shareBadgeID

            anchors {
                top: parent.top
                right: parent.right
                margins: 6
            }

            width: 28
            height: 28
            radius: width / 4
            color: Colors.palette.toolbar
            opacity: 0.92
            border {
                color: Colors.palette.border
                width: 1
            }

            Image {
                anchors.fill: parent
                anchors {
                    leftMargin: 5
                    topMargin: 6
                    rightMargin: 7
                    bottomMargin: 6
                }
                source: "../resources/share.png"
                fillMode: Image.PreserveAspectFit
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: guiController.ShareImage()
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
        visible: !rootID.controlsHidden
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

                rootID.controlsHidden = true
                if (!window) {
                    console.error("Could not find window to capture")
                    rootID.controlsHidden = false
                } else {
                    window.grabToImage((result) => {
                        const savedImageUrl = guiController.SaveImage(result)
                        if (savedImageUrl.length > 0) 
                            rootID.animateThumbnail(savedImageUrl)
                         else 
                            rootID.controlsHidden = false
                    })
                }
            }
        }
    }
}
