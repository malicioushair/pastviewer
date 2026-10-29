import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

import "../GuiItems"
import "../Helpers"
import "../Helpers/colors.js" as Colors
import "Helpers"

import TourController
import PastViewer

BasePage {
    id: rootID

    required property var tourStop
    property bool audioControlsOpen: false
    property bool narrationActive: false
    readonly property bool hasNarration: rootID.tourStop.audioUrl.length > 0

    title: qsTr("Stop Details")

    function stopNarration() {
        narrationActive = false
        narrationPlayerID.stop()
    }

    StackView.onDeactivating: stopNarration()
    Component.onDestruction: narrationPlayerID.stop()

    MediaPlayer {
        id: narrationPlayerID

        audioOutput: AudioOutput {}
        loops: MediaPlayer.Infinite

        onMediaStatusChanged: function(status) {
            if (false
                || !rootID.narrationActive
                || status !== MediaPlayer.EndOfMedia
                || duration <= 0)
                return

            position = 0
            play()
        }
    }

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
    }

    Connections {
        target: GuiController

        function onOnboardingReset() {
            stopInfoOnboardingID.currentIndex = 0
            stopInfoOnboardingID.active = !GuiController.IsOnboardingStepCompleted(stopInfoOnboardingID.completionKey)
        }
    }

    OnboardingOverlay {
        id: stopInfoOnboardingID

        anchors.fill: parent
        z: 1000

        completionKey: "TourStopInfoIntro"
        active: !GuiController.IsOnboardingStepCompleted(completionKey)
        topped: currentIndex > 0

        highlightSteps: {
            const highlights = []
            if (rootID.hasNarration)
                highlights.push({ target: rootID.audioControlsOpen ? playPauseButtonID : listenButtonID, stepIndex: 1 })
            highlights.push({ target: nextStopButtonID, stepIndex: rootID.hasNarration ? 2 : 1 })
            return highlights
        }

        steps: {
            const tutorialSteps = [
                {
                    "title": qsTr("Explore this stop"),
                    "body": rootID.tourStop.imageUrls.length > 1
                        ? qsTr("Swipe through the photos and read the description to learn about this place. Double-tap a photo to open it for a closer look.")
                        : qsTr("Read the description to learn about this place. Double-tap the photo to open it for a closer look.")
                }
            ]
            if (rootID.hasNarration) {
                tutorialSteps.push({
                    "title": qsTr("Listen to the story"),
                    "body": qsTr("Tap Listen to play the narration. You can then pause or resume playback and drag the slider to move through the recording.")
                })
            }
            tutorialSteps.push({
                "title": qsTr("Continue your tour"),
                "body": qsTr("Tap the right arrow to return to the map and select the next stop. Audio stops when you leave this page.")
            })
            return tutorialSteps
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true

            ListView {
                id: stopImagesViewID

                anchors.fill: parent
                model: rootID.tourStop.imageUrls
                orientation: ListView.Horizontal
                spacing: 8
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                snapMode: ListView.SnapOneItem
                highlightRangeMode: ListView.StrictlyEnforceRange
                preferredHighlightBegin: 0
                preferredHighlightEnd: 0
                highlightMoveDuration: 0


                delegate: Item {
                    id: delegateID

                    required property var modelData

                    width: stopImagesViewID.width
                    height: stopImagesViewID.height

                    Image {
                        id: stopImageID

                        anchors.fill: parent

                        source: delegateID.modelData
                        fillMode: Image.PreserveAspectFit
                        visible: status === Image.Ready

                        TapHandler {
                            onDoubleTapped: mainWindowID.openInspectPhotos(parent.source)
                        }
                    }

                    StyledBusyIndicator {
                        anchors.centerIn: parent

                        implicitHeight: 80
                        implicitWidth: implicitHeight
                        visible: stopImageID.status !== Image.Ready
                    }
                }
            }

            Label {
                anchors {
                    top: parent.top
                    right: parent.right
                    margins: 10
                }

                visible: stopImagesViewID.count > 1
                text: qsTr("%1 / %2").arg(stopImagesViewID.currentIndex + 1).arg(stopImagesViewID.count)
                color: "white"
                font.bold: true
                padding: 8

                background: Rectangle {
                    radius: 8
                    color: Colors.palette.seeThroughBlack
                }
            }
        }

        PageIndicator {
            Layout.alignment: Qt.AlignHCenter

            // Keep long galleries compact; the counter remains visible.
            visible: count > 1 && count <= 7
            count: stopImagesViewID.count
            currentIndex: stopImagesViewID.currentIndex

            delegate: Rectangle {
                required property int index

                implicitWidth: 8
                implicitHeight: 8
                radius: 4
                color: index === stopImagesViewID.currentIndex
                    ? Colors.palette.accent : Colors.palette.toolbar
            }
        }

        Label {
            Layout.fillWidth: true

            text: rootID.tourStop.title
            font.bold: true
            font.pixelSize: 20
        }

        Label {
            Layout.fillWidth: true

            text: rootID.tourStop.description
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            StyledButton {
                id: listenButtonID

                Layout.preferredHeight: 50

                visible: !rootID.audioControlsOpen && rootID.hasNarration
                text: qsTr("Listen")
                onClicked: {
                    rootID.audioControlsOpen = true
                    rootID.narrationActive = true
                    narrationPlayerID.source = rootID.tourStop.audioUrl
                    narrationPlayerID.play()
                }
            }

            Button {
                id: playPauseButtonID

                readonly property bool playing: narrationPlayerID.playbackState === MediaPlayer.PlayingState

                Layout.preferredWidth: 50
                Layout.preferredHeight: 50

                visible: rootID.audioControlsOpen
                padding: 0
                text: playing ? "\u23F8" : "\u25B6" // pause and play icons

                background: Rectangle {
                    radius: width / 2
                    color: playPauseButtonID.pressed ? Colors.palette.accentAlt : Colors.palette.accent
                    border.color: Colors.palette.border
                    border.width: 2

                    Behavior on color {
                        ColorAnimation {
                            duration: 150
                        }
                    }
                }

                contentItem: Text {
                    text: playPauseButtonID.text
                    color: "white"
                    font.pixelSize: 18
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (playPauseButtonID.playing)
                        narrationPlayerID.pause()
                    else
                        narrationPlayerID.play()
                }
            }

            Slider {
                id: timelineSliderID

                visible: rootID.audioControlsOpen
                Layout.fillWidth: true
                Layout.preferredHeight: 50

                from: 0
                to: narrationPlayerID.duration > 0 ? narrationPlayerID.duration : 1
                enabled: narrationPlayerID.seekable && narrationPlayerID.duration > 0

                Binding on value {
                    value: narrationPlayerID.position
                    when: !timelineSliderID.pressed
                }

                onMoved: narrationPlayerID.position = value

                background: StyledSliderTrack {
                    control: timelineSliderID
                    fillTo: timelineSliderID.visualPosition
                }

                handle: StyledSliderHandle {
                    control: timelineSliderID
                    pressed: timelineSliderID.pressed
                    visualPosition: timelineSliderID.visualPosition
                }
            }

            Item {
                visible: !rootID.audioControlsOpen
                Layout.fillWidth: true
            }

            StyledButton {
                id: nextStopButtonID

                Layout.preferredHeight: 50

                text: qsTr(">")
                onClicked: {
                    rootID.goBack()
                    TourController.HighlightNextStop()
                }
            }
        }
    }
}
