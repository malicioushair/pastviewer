import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Shapes

import PastViewer 1.0

import "ErrorMessageDialog"
import "GuiItems"

import "Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    color: Colors.palette.bg

    StackView {
        id: stackViewID

        function openPhotoDetails(photo, title, year) {
            stackViewID.push("Views/PhotoDetails.qml", {
                imageSource: photo,
                title: title,
                year: year
            })
        }

        function openSettings() {
            stackViewID.push("Views/Settings.qml")
        }

        anchors.fill: parent

        initialItem: mapPageID
    }

    ErrorMessageDialog {
        id: errorDialogID

        anchors.centerIn: Overlay.overlay
    }

    Connections {
        target: guiController

        function onShowErrorDialog(message) {
            errorDialogID.errorMessage = message
            errorDialogID.open()
        }
    }

    Component {
        id: mapPageID

        ColumnLayout {
            readonly property var positionSource: pastVuModelController.GetPositionSource()

            anchors.fill: parent

            Rectangle {
                id: noLocationWarningID

                Layout.alignment: Qt.AlignHCenter
                Layout.margins: 8

                Layout.preferredWidth: warningTextID.implicitWidth + 64
                Layout.preferredHeight: warningTextID.implicitHeight + 16

                visible: !positionSource.positionAvailable

                radius: 8
                color: Colors.palette.bg
                border.color: Colors.palette.accent
                border.width: 1

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Text {
                        text: "⚠"
                        font.pixelSize: 16
                        color: Colors.palette.accent
                    }

                    Text {
                        id: warningTextID

                        text: qsTr("Current position is not available")
                        color: Colors.palette.text
                        font.pixelSize: 14
                    }

                    Text {
                        text: "⚠"
                        font.pixelSize: 16
                        color: Colors.palette.accent
                    }
                }
            }

            MapView {
                id: mapViewID

                Layout.fillWidth: true
                Layout.fillHeight: true

                Compass {
                    id: compassID

                    anchors {
                        top: parent.top
                        right: parent.right
                        margins: 12
                    }

                    width: 48
                    height: 48
                    z: 999   // above the map
                }

                SettingsButton {
                    id: settingsButtonID

                   anchors {
                        top: parent.top
                        left: parent.left
                        margins: 12
                    }

                    z: 999   // above the map

                    onClicked: stackViewID.openSettings()
                }

                Map {
                    id: mapID

                    property geoCoordinate startCentroid
                    property bool follow: true

                    Timer {
                        id: mapMovementTimerID
                        interval: 300  // Wait 300ms after map stops moving // @TODO figure out how to know when the pan is stopped
                        onTriggered: mapID.updateViewCoordinates()
                    }

                    function scheduleViewUpdate() {
                        mapMovementTimerID.restart()
                    }

                    function updateViewCoordinates() {
                        if (mapID.width <= 0 || mapID.height <= 0)
                            return

                        const topLeftCoord = mapID.toCoordinate(Qt.point(0, 0), false)
                        const bottomRightCoord = mapID.toCoordinate(Qt.point(mapID.width, mapID.height), false)
                        pastVuModelController.SetViewportCoordinates(QtPositioning.rectangle(topLeftCoord, bottomRightCoord))
                    }

                    anchors.fill: parent
                    copyrightsVisible: false
                    plugin: Plugin {
                        id: mapPluginID

                        name: "osm"

                        PluginParameter {
                            name: "osm.mapping.custom.host"
                            value: "https://tiles.stadiamaps.com/tiles/outdoors/%z/%x/%y.png?api_key=" + pastVuModelController.GetMapHostApiKey()
                        }
                    }

                    activeMapType: {
                        const customMapType = supportedMapTypes.find((map) => { return map.style === MapType.CustomMap });
                        if (customMapType)
                            return customMapType;
                        else
                            console.warn("CustomMap not provided by this plugin.");
                    }

                    zoomLevel: pastVuModelController.zoomLevel

                    onCenterChanged: scheduleViewUpdate()
                    onZoomLevelChanged: scheduleViewUpdate()
                    onBearingChanged: scheduleViewUpdate()

                    Binding {
                        id: followCenterID

                        target: mapID
                        property: "center"
                        value: positionSource.coordinate
                        when: mapID.follow && positionSource.coordinate.isValid
                        restoreMode: Binding.RestoreNone
                    }

                    MapItemView {
                        id: mapItemViewID

                        model: pastVuModelController.model
                        delegate: delegateID
                    }

                    Component {
                        id: delegateID

                        MapQuickItem {
                            z: Selected ? 1 : 0
                            coordinate: model.Coordinate
                            anchorPoint: Qt.point(povDirectionID.width / 2, povDirectionID.height)

                            sourceItem: PovDirection {
                                id: povDirectionID

                                size: 20
                                bearing: model.Bearing - compassID.bearing
                                mapBearing: mapID.bearing
                                selected: model.Selected

                                onClicked: stackViewID.openPhotoDetails(model.Photo, model.Title, model.Year)
                            }
                        }
                    }

                    RecenterButton {
                        id: recenterID

                        anchors {
                            bottom: parent.bottom
                            right: parent.right
                            margins: 12
                        }

                        height: 48
                        width: 96

                        onTapped: mapID.follow = true
                    }

                    // Drag to pan (continuous)
                    DragHandler {
                        id: mapDragID

                        dragThreshold: 20
                        grabPermissions: PointerHandler.CanTakeOverFromAnything
                        xAxis.onActiveValueChanged: (dx) => target.pan(-dx, 0)
                        yAxis.onActiveValueChanged: (dy) => target.pan(0, -dy)

                        onActiveChanged: {
                            if (active)
                                mapID.follow = false
                        }
                    }

                    PinchHandler {
                        id: pinchHandlerID

                        target: null
                        onActiveChanged: {
                            if (active)
                                mapID.follow = false
                            mapID.startCentroid = mapID.toCoordinate(pinchHandlerID.centroid.position, false)
                        }
                        onScaleChanged: (delta) => {
                            mapID.zoomLevel += Math.log2(delta)
                            pastVuModelController.zoomLevel = mapID.zoomLevel // @TODO think on removing the repetirion

                            mapID.alignCoordinateToPoint(mapID.startCentroid, pinchHandlerID.centroid.position)
                        }
                        onRotationChanged: (delta) => {
                            mapID.bearing -= delta
                            mapID.alignCoordinateToPoint(mapID.startCentroid, pinchHandlerID.centroid.position)
                        }
                        grabPermissions: PointerHandler.TakeOverForbidden
                    }

                    TapHandler {
                        id: mapTapHandlerID

                        target: null
                        onDoubleTapped: {
                            mapID.follow = false
                            ++mapID.zoomLevel
                            mapID.center = mapID.toCoordinate(mapTapHandlerID.point.position)
                        }
                    }

                    MapQuickItem {
                        id: youAreHereMarkerID

                        anchorPoint: Qt.point(sourceItem.width / 2, sourceItem.height / 2)
                        coordinate: positionSource.coordinate
                        visible: coordinate.isValid
                        sourceItem: UserPosition {
                            bearing: positionSource.bearing
                            mapBearing: mapID.bearing
                        }
                    }
                }
            }

            PhotosNear {
                id: photosNearID
                map: mapID
            }
        }
    }
}
