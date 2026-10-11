import QtQuick
import QtLocation

import PastViewer 1.0

import "../Helpers/colors.js" as Colors

MapPolyline {
    id: rootID

    path: GuiController.currentTour.tourPath
    line.width: 4
    line.color: Colors.palette.accent

    visible: path.length >= 2
}
