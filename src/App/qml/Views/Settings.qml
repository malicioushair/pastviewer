import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors
import "../GuiItems"
import "Helpers"


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
        text: qsTr("Version: ") + guiController.GetAppVersion()
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 20
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignTop

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

                    Layout.fillWidth: true
                    model: i18nController.languageModel
                    textRole: "NameRole"
                    valueRole: "CodeRole"
                    currentIndex: {
                        const index = i18nController.GetIndexOf(i18nController.GetCurrentLanguage())
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

                    Component.onCompleted: currentValue = i18nController.GetCurrentLanguage()

                    Connections {
                        target: i18nController
                        function onLanguageChanged() {
                            currentValue = i18nController.GetCurrentLanguage()
                        }
                    }

                    onActivated: i18nController.SetCurrentLanguage(languageComboBoxID.currentValue);
                }
            }

            SettingWithHint {
                Layout.leftMargin: -7

                description: qsTr("When enabled, the map shows only historical photos near your current location.")

                StyledCheckBox {
                    checked: pastVuModelController.nearestObjectsOnly
                    text: qsTr("Show only nearest objects")
                    onClicked: pastVuModelController.ToggleOnlyNearestObjects();
                }
            }

            SettingWithHint {
                Layout.leftMargin: -7

                description: qsTr('When enabled, the History near you row lists all photos in the map area within the timeline. When disabled, only nearby photos are shown.')

                StyledCheckBox {
                    checked: pastVuModelController.historyNearModelType
                    text: qsTr('Show all objects in "History near you"')
                    onClicked: pastVuModelController.ToggleHistoryNearYouModel();
                }
            }


            StyledRangeSlider {
                id: timelineSettingID

                Layout.rightMargin: 18

                rangeMin: pastVuModelController.timelineRange.min
                rangeMax: pastVuModelController.timelineRange.max
                selectedMin: pastVuModelController.userSelectedTimelineRange.min
                selectedMax: pastVuModelController.userSelectedTimelineRange.max

                onSelectedMinChanged: pastVuModelController.userSelectedTimelineRange.min = selectedMin
                onSelectedMaxChanged: pastVuModelController.userSelectedTimelineRange.max = selectedMax
            }

            SettingWithHint {
                description: qsTr("Show the introductory tips on the map and photo screens again.")

                StyledButton {
                    id: resetOnboardingID

                    text: qsTr("Reset onboarding")
                    onClicked: guiController.ResetOnboarding()
                }
            }

            SettingWithHint {
                description: qsTr("Load historical photos again for the current map view using your current filters.")

                StyledButton {
                    id: reloadButtonID

                    text: qsTr("Reload map items")
                    onClicked: pastVuModelController.ReloadItems()
                }
            }
        }
    }
}
