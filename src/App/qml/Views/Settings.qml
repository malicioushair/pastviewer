import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors
import "../GuiItems"
import "Helpers"

import PastViewer

BasePage {
    id: rootID

    title: qsTr("Settings")

    header: Header {
        label.font {
            bold: true
            pixelSize: 16
        }
    }

    footer: Footer {
        text: qsTr("Version: ") + GuiController.GetAppVersion()
    }

    ScrollView {
        id: settingsScrollViewID

        anchors {
            fill: parent
            margins: 20
        }
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: settingsScrollViewID.availableWidth
            spacing: 20

            ColumnLayout {
                id: i18nBlockID

                Layout.rightMargin: 13
                Layout.fillWidth: true
                spacing: 5

                Label {
                    text: qsTr("Language")
                    color: Colors.palette.text
                    font {
                        bold: true
                        pixelSize: 14
                    }
                }

                ComboBox {
                    id: languageComboBoxID

                    function setLanguage(language) {
                        const value = language
                        const index = languageComboBoxID.indexOfValue(value)
                        languageComboBoxID.currentIndex = index
                    }

                    Layout.fillWidth: true
                    model: I18nController.languageModel
                    textRole: "NameRole"
                    valueRole: "CodeRole"
                    currentIndex: {
                        const index = I18nController.GetIndexOf(I18nController.GetCurrentLanguage())
                        if (index >= 0)
                            return index
                    }

                    background: Rectangle {
                        color: Colors.palette.bg
                        border {
                            color: Colors.palette.border
                            width: 1
                        }
                        radius: 8
                    }

                    contentItem: Text {
                        text: languageComboBoxID.displayText
                        color: Colors.palette.text
                        font.pixelSize: 14
                        leftPadding: 12
                        verticalAlignment: Text.AlignVCenter
                    }

                    popup: Popup {
                        y: languageComboBoxID.height
                        width: languageComboBoxID.width
                        implicitHeight: contentItem.implicitHeight
                        padding: 1

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: languageComboBoxID.popup.visible ? languageComboBoxID.delegateModel : null
                            currentIndex: languageComboBoxID.highlightedIndex

                            ScrollIndicator.vertical: ScrollIndicator { }
                        }

                        background: Rectangle {
                            color: Colors.palette.bg
                            border {
                                color: Colors.palette.border
                                width: 1
                            }
                            radius: 8
                        }
                    }

                    delegate: ItemDelegate {
                        width: languageComboBoxID.width
                        text: model.NameRole

                        background: Rectangle {
                            color: parent.hovered ? Colors.palette.accentAlt : Colors.palette.bg
                            radius: 4
                        }

                        contentItem: Text {
                            text: model.NameRole
                            color: Colors.palette.text
                            font.pixelSize: 14
                            leftPadding: 12
                            verticalAlignment: Text.AlignVCenter
                        }
                    }

                    Component.onCompleted: {
                        languageComboBoxID.setLanguage(I18nController.GetCurrentLanguage())
                    }

                    Connections {
                        target: I18nController
                        function onCurrentLanguageChanged() {
                            languageComboBoxID.setLanguage(I18nController.GetCurrentLanguage())
                        }
                    }

                    onActivated: I18nController.SetCurrentLanguage(languageComboBoxID.currentValue);
                }
            }

            SettingWithHint {
                Layout.leftMargin: -7

                description: qsTr("When enabled, the map shows only historical photos near your current location.")

                StyledCheckBox {
                    checked: PastVuModelController.nearestObjectsOnly
                    text: qsTr("Show only nearest objects")
                    onClicked: PastVuModelController.ToggleOnlyNearestObjects();
                }
            }

            SettingWithHint {
                Layout.leftMargin: -7

                description: qsTr('When enabled, the History near you row lists all photos in the map area within the timeline. When disabled, only nearby photos are shown.')

                StyledCheckBox {
                    checked: PastVuModelController.historyNearModelType
                    text: qsTr('Show all objects in "History near you"')
                    onClicked: PastVuModelController.ToggleHistoryNearYouModel();
                }
            }

            SettingWithHint {
                Layout.leftMargin: -7

                description: qsTr("When enabled, PastViewer notifies you when you walk near a historical photo.")

                StyledCheckBox {
                    checked: PastVuModelController.proximityNotificationsEnabled
                    text: qsTr("Proximity notifications")
                    onClicked: PastVuModelController.ToggleProximityNotifications();
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.rightMargin: 18
                spacing: 8
                enabled: PastVuModelController.proximityNotificationsEnabled
                opacity: enabled ? 1.0 : 0.45

                Text {
                    text: qsTr("Notification distance: %1 m").arg(proximityDistanceSliderID.value)
                    color: Colors.palette.text
                    font.family: "monospace"
                }

                Slider {
                    id: proximityDistanceSliderID

                    Layout.fillWidth: true

                    from: PastVuModelController.GetNotificationDistanceMin()
                    to: PastVuModelController.GetNotificationDistanceMax()
                    stepSize: 5
                    snapMode: Slider.SnapAlways
                    value: PastVuModelController.proximityNotificationDistance
                    onMoved: PastVuModelController.proximityNotificationDistance = Math.round(value)

                    background: Rectangle {
                        x: proximityDistanceSliderID.leftPadding
                        y: proximityDistanceSliderID.topPadding + proximityDistanceSliderID.availableHeight / 2 - height / 2
                        width: proximityDistanceSliderID.availableWidth
                        height: 8
                        radius: 4
                        color: Colors.palette.sliderAlt

                        Rectangle {
                            width: proximityDistanceSliderID.visualPosition * parent.width
                            height: parent.height
                            color: Colors.palette.slider
                            radius: 4
                        }
                    }

                    handle: StyledSliderHandle {
                        yOffset: 10

                        pressed: proximityDistanceSliderID.pressed
                        visualPosition: proximityDistanceSliderID.visualPosition
                        availableWidth: proximityDistanceSliderID.availableWidth
                    }
                }
            }

            StyledRangeSlider {
                id: timelineSettingID

                Layout.rightMargin: 18

                rangeMin: PastVuModelController.timelineRange.min
                rangeMax: PastVuModelController.timelineRange.max
                selectedMin: PastVuModelController.userSelectedTimelineRange.min
                selectedMax: PastVuModelController.userSelectedTimelineRange.max

                onSelectedMinChanged: PastVuModelController.userSelectedTimelineRange.min = selectedMin
                onSelectedMaxChanged: PastVuModelController.userSelectedTimelineRange.max = selectedMax
            }

            SettingWithHint {
                visible: GuiController.HasTipsUrl()
                description: qsTr("By leaving a tip, you support PastViewer's continued development and keep it FREE")

                StyledButton {
                    id: supportButtonID

                    text: qsTr("Support us 💰")
                    onClicked: GuiController.OpenTipsUrl()
                }
            }

            SettingWithHint {
                description: qsTr("Show the introductory tips on the map and photo screens again.")

                StyledButton {
                    id: resetOnboardingID

                    text: qsTr("Reset onboarding")
                    onClicked: GuiController.ResetOnboarding()
                }
            }

            SettingWithHint {
                description: qsTr("Load historical photos again for the current map view using your current filters.")

                StyledButton {
                    id: reloadButtonID

                    text: qsTr("Reload map items")
                    onClicked: PastVuModelController.ReloadItems()
                }
            }
        }
    }
}
