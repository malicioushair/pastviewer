import QtQuick
import QtLocation

import TourController

import Colors

MapPolyline {
    id: rootID

    path: TourController.currentTour.tourPath
    line.width: 4
    line.color: Colors.accent

    visible: path.length >= 2
}
