import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia

import "../GuiItems"
import "../Helpers/colors.js" as Colors
import "Helpers"

import TourController

BasePage {
    id: rootID

    required property var tourStop
    property bool audioControlsOpen: false
    property bool narrationActive: false

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

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }

        Image {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter

            source: rootID.tourStop.imageUrl
            fillMode: Image.PreserveAspectFit

            TapHandler {
                onDoubleTapped: {
                    mainWindowID.openInspectPhotos(parent.source)
                }
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
                Layout.preferredHeight: 50

                visible: !rootID.audioControlsOpen && rootID.tourStop.audioUrl.length > 0
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
