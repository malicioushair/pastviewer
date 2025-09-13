import QtMultimedia
import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

Video {
    id: videoID

    focus: true

    Keys.onSpacePressed: videoID.playbackState === MediaPlayer.PlayingState ? videoID.pause() : videoID.play()
    Keys.onLeftPressed: videoID.position = videoID.position - 5000
    Keys.onRightPressed: videoID.position = videoID.position + 5000
    Keys.onEscapePressed: {
        if (mainWindowID.visibility === Window.FullScreen)
            mainWindowID.visibility = Window.Windowed
    }

    Connections {
        target: guiController
        function onReadyToPlayVideo() {
            videoID.source = guiController.videoFile
            if (videoID.playbackState !== MediaPlayer.PlayingState)
                videoID.play()
        }
    }

    FileDialog {
        id: fileDialogID
        onAccepted: {
            guiController.DownloadWithTorrentFile(selectedFile)

            videoID.source = guiController.videoFile
            // calling the play and pause to make it obvious that a video was added
            videoID.play()
            videoID.pause()
        }
    }

    MouseArea {
        id: videoAreaID

        anchors.fill: parent

        onDoubleClicked: {
            mainWindowID.visibility = mainWindowID.visibility === Window.FullScreen ? Window.Windowed : Window.FullScreen
            controlsID.visible = !controlsID.visible
        }
        onPositionChanged: {
            controlsID.visible = true
            hideControlsTimerID.restart()
        }
    }

    Timer {
        id: hideControlsTimerID

        interval: 3000
        onTriggered: controlsID.visible = mainWindowID.visibility !== Window.FullScreen
    }

    Item {
        id: controlsID

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: 90

        ColumnLayout {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right

            width: parent.width

            Rectangle {
                Layout.fillWidth: true

                height: 40

                color: "transparent"

                ProgressBar {
                    id: downloadProgressID

                    anchors.fill: parent

                    from: 0
                    value: guiController.downloadProgress
                    to: 100
                }

                Slider {
                    id: seekBarID

                    anchors.fill: parent

                    from: 0
                    value: videoID.position
                    to: videoID.duration

                    onMoved: videoID.seek(seekBarID.value)
                }

            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: -10
                Layout.bottomMargin: 10

                Slider {
                    id: volumeControlID

                    from: 0.0
                    value: videoID.volume
                    to: 1.0

                    onMoved: videoID.volume = volumeControlID.value
                }

                Button {
                    id: playButtonID

                    text: videoID.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                    onClicked: videoID.playbackState === MediaPlayer.PlayingState ? videoID.pause() : videoID.play()
                }

                Button {
                    id: stopButtonID

                    text: "◼"
                    onClicked: videoID.stop()
                }

                Button {
                    id: openFileButtonID

                    text: "📂"
                    onClicked: fileDialogID.open()
                }

                Label {
                    id: currentTimeID

                    Layout.fillWidth: true

                    text: `${Math.floor(videoID.position / 60000)}:${Math.floor((videoID.position % 60000) / 1000).toString().padStart(2, '0')}`
                }

                Label {
                    id: downloadPercentageID

                    text: `${downloadProgressID.value}%`
                }
            }
        }
    }
}