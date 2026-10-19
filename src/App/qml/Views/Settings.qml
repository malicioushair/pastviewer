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
        TapHandler {
            property int adminCounter: 0
            onTapped: {
                if (++adminCounter > 15) {
                    GuiController.isAdmin = !GuiController.isAdmin
                    adminCounter = 0
                }
            }
        }
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

                LanguageComboBox {
                    id: languageComboBoxID

                    Layout.fillWidth: true
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

                    background: StyledSliderTrack {
                        control: proximityDistanceSliderID
                        fillTo: proximityDistanceSliderID.visualPosition
                    }

                    handle: StyledSliderHandle {
                        control: proximityDistanceSliderID
                        pressed: proximityDistanceSliderID.pressed
                        visualPosition: proximityDistanceSliderID.visualPosition
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
                visible: GuiController.HasTipSupport()
                description: qsTr("By leaving a tip, you support PastViewer's continued development and keep it FREE")

                StyledButton {
                    id: supportButtonID

                    text: qsTr("Support us 💰")
                    onClicked: GuiController.RequestTipFlow()
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

            SettingWithHint {
                description: qsTr("Browse and create tours")
                visible: GuiController.isAdmin && PastVuModelController.hasPositionSource

                StyledButton {
                    id: toursID

                    text: qsTr("Tours")
                    onClicked: mainWindowID.openTours()
                }
            }
        }
    }
}
