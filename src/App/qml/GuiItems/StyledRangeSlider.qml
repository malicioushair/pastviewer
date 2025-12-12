import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors

ColumnLayout {
    id: timelineSettingID

    Layout.leftMargin: 5
    Layout.rightMargin: 5

    property real rangeMin: 0
    property real rangeMax: 100
    property real selectedMin: 0
    property real selectedMax: 100

    Text {
        text: qsTr("Timeline: ") + Math.floor(timelineSliderID.first.value) + " - " + Math.floor(timelineSliderID.second.value)
        font.family: "monospace"
    }

    RangeSlider {
        id: timelineSliderID

        Layout.fillWidth: true

        from: rangeMin
        to: rangeMax
        first.value: selectedMin
        second.value: selectedMax
        first.onMoved: selectedMin = Math.round(first.value)
        second.onMoved: selectedMax = Math.round(second.value)

        background: Item {
            implicitHeight: 8
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right

                x: timelineSliderID.first.visualPosition * parent.width
                width: (timelineSliderID.second.visualPosition - timelineSliderID.first.visualPosition) * parent.width
                height: 8

                color: Colors.palette.slider
                radius: 4
            }
        }

        first.handle: StyledSliderHandle {
            pressed: timelineSliderID.first.pressed
            visualPosition: timelineSliderID.first.visualPosition
            availableWidth: timelineSliderID.availableWidth
            xOffset: -2
        }

        second.handle: StyledSliderHandle {
            pressed: timelineSliderID.second.pressed
            visualPosition: timelineSliderID.second.visualPosition
            availableWidth: timelineSliderID.availableWidth
        }
    }
}

