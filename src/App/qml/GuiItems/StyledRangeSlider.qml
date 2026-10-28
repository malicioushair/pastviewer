import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

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

        background: StyledSliderTrack {
            control: timelineSliderID
            fillFrom: timelineSliderID.first.visualPosition
            fillTo: timelineSliderID.second.visualPosition
        }

        first.handle: StyledSliderHandle {
            xOffset: -5
            control: timelineSliderID
            pressed: timelineSliderID.first.pressed
            visualPosition: timelineSliderID.first.visualPosition
        }

        second.handle: StyledSliderHandle {
            control: timelineSliderID
            pressed: timelineSliderID.second.pressed
            visualPosition: timelineSliderID.second.visualPosition
        }
    }
}
