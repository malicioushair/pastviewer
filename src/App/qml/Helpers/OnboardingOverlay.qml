import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "colors.js" as Colors
import "../GuiItems"

Item {
    id: rootID

    // Array of step objects: { title, body }
    property var steps: []
    property int currentIndex: 0
    property bool centered: false
    property bool topped: false

    // Array of { target: Item, stepIndex: int } — when currentIndex === stepIndex, cut a hole to highlight target
    property var highlightSteps: []

    required property string completionKey

    function _highlightTargetForCurrentStep() {
        for (var i = 0; i < highlightSteps.length; i++) {
            if (highlightSteps[i].stepIndex === currentIndex)
                return highlightSteps[i].target
        }
        return null
    }

    readonly property Item _currentHighlightTarget: _highlightTargetForCurrentStep()
    readonly property bool _showHighlight: _currentHighlightTarget && _currentHighlightTarget.parent
    readonly property rect _holeRect: {
        const coords = rootID.parent.mapFromItem(_currentHighlightTarget, 0, 0)
        return _showHighlight
        ? Qt.rect(coords.x,
                  coords.y,
                  _currentHighlightTarget.width,
                  _currentHighlightTarget.height)
        : Qt.rect(0, 0, 0, 0)
    }

    // Whether the overlay is currently active. When false, it is hidden and does not block input.
    property bool active: false

    visible: active
    enabled: active

    // Full overlay or frame with hole to highlight target area
    Item {
        id: overlayContainerID
        anchors.fill: parent

        Rectangle {
            visible: !rootID._showHighlight
            anchors.fill: parent
            color: Colors.palette.overlayDim
        }

        RectangleWithHoles {
            visible: rootID._showHighlight
            anchors.fill: parent
            holes: rootID._showHighlight ? [rootID._holeRect] : []
            overlayColor: Colors.palette.overlayDim
            highlightBorderColor: Colors.palette.accent
            highlightBorderWidth: 2
            holeRadius: 8
        }
    }

    Rectangle {
        id: cardID

        width: Math.min(parent.width * 0.9, 420)
        implicitHeight: contentLayout.implicitHeight + 24

        radius: 16
        color: Colors.palette.toolbar
        border.color: Colors.palette.border
        border.width: 1

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            bottomMargin: 32
        }
        states: [
            State {
                name: "centered"
                when: rootID.centered
                AnchorChanges {
                    target: cardID
                    anchors.bottom: undefined
                    anchors.verticalCenter: rootID.verticalCenter
                }
            },
            State {
                name: "topped"
                when: rootID.topped
                AnchorChanges {
                    target: cardID
                    anchors {
                        bottom: undefined
                        top: parent.top
                    }
                }
                PropertyChanges {
                    target: cardID
                    anchors.topMargin: 70
                }
            }
        ]
        transitions: Transition {
            AnchorAnimation {
                duration: 200
            }
        }
        ColumnLayout {
            id: contentLayout

            anchors {
                fill: parent
                margins: 16
            }
            spacing: 12

            Text {
                id: titleTextID

                Layout.fillWidth: true

                text: steps.length > 0 ? steps[Math.min(currentIndex, steps.length - 1)].title : ""
                wrapMode: Text.WordWrap
                color: Colors.palette.text
                font.pixelSize: 18
                font.bold: true
            }

            Text {
                id: bodyTextID

                Layout.fillWidth: true

                text: steps.length > 0 ? steps[Math.min(currentIndex, steps.length - 1)].body : ""
                wrapMode: Text.WordWrap
                color: Colors.palette.text
                font.pixelSize: 14
            }

            RowLayout {
                Layout.fillWidth: true

                spacing: 8

                Text {
                    Layout.alignment: Qt.AlignVCenter

                    text: steps.length > 1
                          ? qsTr("%1 / %2").arg(currentIndex + 1).arg(steps.length)
                          : ""
                    color: Colors.palette.text
                    font.pixelSize: 12
                }

                Item {
                    Layout.fillWidth: true
                }

                StyledButton {
                    id: skipButtonID

                    visible: steps.length > 1
                    text: qsTr("Skip")

                    onClicked: {
                        guiController.SetOnboardingStepCompleted(completionKey)
                        rootID.active = false
                    }
                }

                StyledButton {
                    id: nextButtonID

                    text: currentIndex < steps.length - 1 ? qsTr("Next") : qsTr("Got it")

                    onClicked: {
                        if (currentIndex < steps.length - 1) {
                            currentIndex += 1
                        } else {
                            guiController.SetOnboardingStepCompleted(completionKey)
                            rootID.active = false
                        }
                    }
                }
            }
        }
    }
}

