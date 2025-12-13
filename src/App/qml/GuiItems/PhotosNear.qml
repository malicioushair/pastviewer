import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtLocation
import QtPositioning

import "../Helpers/colors.js" as Colors
import "../Helpers/utils.js" as Utils

Rectangle {
    id: rootID

    property Map map: null

    Layout.fillWidth: true
    Layout.preferredHeight: 210

    radius: 16
    color: Colors.palette.bg

    property real animatedLat: 0
    property real animatedLon: 0

    function animateMapCenter(targetCoordinate) {
        if (!map || !targetCoordinate || !targetCoordinate.isValid)
            return

        map.follow = false

        animatedLat = map.center.latitude
        animatedLon = map.center.longitude

        mapCenterAnimationLat.from = map.center.latitude
        mapCenterAnimationLat.to = targetCoordinate.latitude
        mapCenterAnimationLon.from = map.center.longitude
        mapCenterAnimationLon.to = targetCoordinate.longitude

        mapCenterAnimationGroup.start()
    }

    ParallelAnimation {
        id: mapCenterAnimationGroup

        NumberAnimation {
            id: mapCenterAnimationLat
            target: rootID
            property: "animatedLat"
            duration: 500
            easing.type: Easing.InOutCubic
        }
        NumberAnimation {
            id: mapCenterAnimationLon
            target: rootID
            property: "animatedLon"
            duration: 500
            easing.type: Easing.InOutCubic
        }
    }

    onAnimatedLatChanged: {
        if (map && mapCenterAnimationGroup.running) {
            map.center = QtPositioning.coordinate(animatedLat, animatedLon)
        }
    }

    onAnimatedLonChanged: {
        if (map && mapCenterAnimationGroup.running) {
            map.center = QtPositioning.coordinate(animatedLat, animatedLon)
        }
    }

    Rectangle {
        id: imagesNearbyID

        anchors {
            top: parent.top
            right: parent.right
            margins: 12
        }

        width: 24
        height: 24
        z: 999

        radius: 10

        color: Colors.palette.accentAlt
        border.color: Colors.palette.border

        Text {
            anchors.centerIn: parent
            text: listViewID.model.count
            color: "white"
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }
        spacing: 10

        Text {
            text: qsTr("History near you")
            font.bold: true
            font.pixelSize: 18
        }

        ListView {
            id: listViewID

            Layout.fillWidth: true
            Layout.fillHeight: true


            model: pastVuModelController.historyNearModel
            orientation: ListView.Horizontal
            spacing: 10

            onCountChanged: Utils.setTimeout(positionViewAtEnd, 300)

            delegate: Item {
                width: 100
                height: 160

                Rectangle {
                    id: maskID

                    anchors.fill: parent
                    anchors.margins: -3

                    radius: 10
                    border {
                        width: Selected ? 3 : 0
                        color: Selected ? Colors.palette.accent : "transparent"
                    }
                    color: Selected ? Colors.palette.shadowSoft : Colors.palette.bg
                    antialiasing: true
                }

                ShaderEffectSource {
                    id: maskTextureID

                    sourceItem: maskID
                }

                ColumnLayout {
                    anchors.fill: parent

                    Image {
                        id: imageID

                        Layout.alignment: Qt.AlignTop
                        Layout.fillWidth: true
                        Layout.preferredHeight: 100

                        source: Thumbnail
                        fillMode: Image.PreserveAspectCrop

                        layer.enabled: true
                        layer.effect: MultiEffect {
                                maskEnabled: true
                                maskSource: maskTextureID
                        }

                        TapHandler {
                            onTapped: {
                                Selected = true
                                if (rootID.map && Coordinate && Coordinate.isValid) {
                                    rootID.animateMapCenter(Coordinate)
                                }
                            }
                            onDoubleTapped: stackViewID.openPhotoDetails(Photo, Title, Year)
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 5

                        text: Title
                        maximumLineCount: 2
                        elide: Text.ElideRight
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        Layout.leftMargin: 5
                        Layout.topMargin: -6
                        text: Year
                    }
                }
            }
        }
    }
}