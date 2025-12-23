import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors
import "../GuiItems"

Page {
    id: rootID

    title: "Settings"

    background: Rectangle {
        color: Colors.palette.bg
    }

    header: ToolBar {
        background: Rectangle {
            color: Colors.palette.toolbar
        }
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "←"
                onClicked: rootID.StackView.view.pop()
                background: Rectangle {
                    color: Colors.palette.accent
                }
            }
            Label {
                Layout.fillWidth: true
                text: rootID.title
                color: Colors.palette.text
                wrapMode: Text.Wrap
                font {
                    bold: true
                    pixelSize: 16
                }
            }
        }
    }

    footer: ToolBar {
        background: Rectangle {
            color: Colors.palette.toolbar
        }
        ColumnLayout {
            Label {
                Layout.leftMargin: 10
                text: qsTr("Version: ") + guiController.GetAppVersion()
                color: Colors.palette.text
                wrapMode: Text.WordWrap
            }
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 20
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignTop

            spacing: 20

            StyledCheckBox {
                checked: pastVuModelController.nearestObjectsOnly
                text: qsTr("Show only nearest objects")
                onClicked: pastVuModelController.ToggleOnlyNearestObjects();
            }

            StyledCheckBox {
                checked: pastVuModelController.historyNearModelType
                text: qsTr('Show all objects in "History near you"')
                onClicked: pastVuModelController.ToggleHistoryNearYouModel();
            }

            StyledRangeSlider {
                id: timelineSettingID

                rangeMin: pastVuModelController.timelineRange.min
                rangeMax: pastVuModelController.timelineRange.max
                selectedMin: pastVuModelController.userSelectedTimelineRange.min
                selectedMax: pastVuModelController.userSelectedTimelineRange.max

                onSelectedMinChanged: pastVuModelController.userSelectedTimelineRange.min = selectedMin
                onSelectedMaxChanged: pastVuModelController.userSelectedTimelineRange.max = selectedMax
            }

            Button {
                id: reloadButtonID

                text: qsTr("Reload map items")

                background: Rectangle {
                    color: reloadButtonID.pressed ? Colors.palette.accentAlt : Colors.palette.accent
                    radius: 8
                    border {
                        color: Colors.palette.border
                        width: 2
                    }

                    Behavior on color {
                        ColorAnimation {
                            duration: 150
                        }
                    }
                }

                contentItem: Text {
                    text: reloadButtonID.text
                    font {
                        pixelSize: 16
                        bold: true
                    }
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                padding: 12
                implicitHeight: 44

                onClicked: pastVuModelController.ReloadItems()
            }
        }
    }
}
