import QtQuick
import QtLocation

import TourController

import "../Helpers/colors.js" as Colors

MapPolyline {
    id: rootID

    path: TourController.currentTour.tourPath
    line.width: 4
    line.color: Colors.palette.accent

    visible: path.length >= 2
}
