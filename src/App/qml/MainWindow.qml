import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtLocation
import QtPositioning
import QtQuick.Shapes

import PastViewer 1.0

import "ErrorMessageDialog"
import "GuiItems"
import "Helpers"
import "Views"

import "Helpers/colors.js" as Colors

Rectangle {
    id: mainWindowID

    property alias mapAnimationHelper: mapAnimationHelperID

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

    color: Colors.palette.bg

    MapAnimationHelper {
        id: mapAnimationHelperID

        map: stackViewID.currentItem ? stackViewID.currentItem.map : null
    }

    StackView {
        id: stackViewID

        anchors.fill: parent
        focus: true

        initialItem: MapPage {
            id: mapPageInstance
        }
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

}
