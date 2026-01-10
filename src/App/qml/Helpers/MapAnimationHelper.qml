import QtQuick
import QtLocation
import QtPositioning

import PastViewer 1.0

/**
 * MapAnimationHelper - QML component for animating map center and zoom
 *
 * Provides smooth animations for centering the map and zooming in/out,
 * with support for cluster splitting detection.
 */
Item {
    id: rootID

    visible: false

    required property var map

    // Exposed to make the user code handle the animatedSmthChanged signals
    property real animatedLat: 0
    property real animatedLon: 0
    property real animatedZoom: 0

    property var pendingClusterCheck: null
    readonly property bool running: animationGroup.running

    ParallelAnimation {
        id: animationGroup

        NumberAnimation {
            id: latAnimation
            target: rootID
            property: "animatedLat"
            duration: 500
            easing.type: Easing.InOutCubic
        }

        NumberAnimation {
            id: lonAnimation
            target: rootID
            property: "animatedLon"
            duration: 500
            easing.type: Easing.InOutCubic
        }

        NumberAnimation {
            id: zoomAnimation
            target: rootID
            property: "animatedZoom"
            duration: 500
            easing.type: Easing.InOutCubic
        }

        onFinished: {
            if (rootID.pendingClusterCheck)
                rootID._handleAnimationFinished()
        }
    }


    /**
     * Internal helper: Set up center animation properties
     * @param {Object} targetCoordinate - Target coordinate to center on (null to keep current)
     */
    function _setupCenterAnimation(targetCoordinate) {
        if (!targetCoordinate || !targetCoordinate.isValid)
            return

        animatedLat = map.center.latitude
        animatedLon = map.center.longitude

        latAnimation.from = map.center.latitude
        latAnimation.to = targetCoordinate.latitude
        lonAnimation.from = map.center.longitude
        lonAnimation.to = targetCoordinate.longitude
    }

    /**
     * Internal helper: Set up zoom animation properties
     * @param {number} targetZoom - Target zoom level (0 or negative to keep current)
     */
    function _setupZoomAnimation(targetZoom) {
        animatedZoom = map.zoomLevel

        if (targetZoom > 0 && targetZoom !== map.zoomLevel) {
            zoomAnimation.from = map.zoomLevel
            zoomAnimation.to = targetZoom
        } else {
            // Keep current zoom
            zoomAnimation.from = map.zoomLevel
            zoomAnimation.to = map.zoomLevel
        }
    }

    /**
     * Animation function for center only
     * @param {Object} targetCoordinate - Target coordinate to center on
     */
    function animateMapCenter(targetCoordinate) {
        if (!map || !targetCoordinate || !targetCoordinate.isValid)
            return

        map.follow = false

        _setupCenterAnimation(targetCoordinate)
        _setupZoomAnimation(0)

        animationGroup.start()
    }

    /**
    * Stores cluster info for checking split after animation
    */
    function _handleClustering(targetZoom, clusterCoordinate) {
        pendingClusterCheck = {
            map: map,
            clusterCoordinate: clusterCoordinate,
            currentZoom: targetZoom
        }
    }

    /**
     * Animation function for zoom only
     * @param {number} targetZoom - Target zoom level
     * @param {Object} clusterCoordinate - Original cluster coordinate (for split checking)
     */
    function animateMapZoom(targetZoom, clusterCoordinate) {
        if (!map || targetZoom <= 0 || targetZoom === map.zoomLevel)
            return

        map.follow = false

        _setupZoomAnimation(targetZoom)
        _handleClustering(targetZoom, clusterCoordinate)

        animationGroup.start()
    }

    /**
     * Unified animation function for both center and zoom
     * @param {Object} targetCoordinate - Target coordinate to center on
     * @param {number} targetZoom - Target zoom level (0 to skip zoom animation)
     * @param {Object} clusterCoordinate - Original cluster coordinate (for split checking)
     */
    function animateMapCenterAndZoom(targetCoordinate, targetZoom, clusterCoordinate) {
        if (!map || !targetCoordinate || !targetCoordinate.isValid)
            return

        map.follow = false

        _setupCenterAnimation(targetCoordinate)
        _setupZoomAnimation(targetZoom)
        _handleClustering(targetZoom, clusterCoordinate)

        animationGroup.start()
    }

    /**
     * Handle animation finished - trigger viewport update and check cluster split
     */
    function _handleAnimationFinished() {
        if (!pendingClusterCheck)
            return

        const mapObj = pendingClusterCheck.map
        const topLeftCoord = mapObj.toCoordinate(Qt.point(0, 0), false)
        const bottomRightCoord = mapObj.toCoordinate(Qt.point(mapObj.width, mapObj.height), false)
        pastVuModelController.SetViewportCoordinates(QtPositioning.rectangle(topLeftCoord, bottomRightCoord))

        pendingClusterCheck = null
    }
}

