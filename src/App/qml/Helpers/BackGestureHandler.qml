import QtQuick
import QtQuick.Controls

Item {
    id: rootID

    required property var stackView

    property var outgoing: null
    property var incoming: null
    property var savedIncoming: null
    property real outgoingX: 0
    property real progress: 0
    property bool completing: false
    readonly property bool tracking: outgoing !== null

    function begin() {
        outgoing = rootID.stackView.currentItem
        incoming = rootID.stackView.get(rootID.stackView.depth - 2)
        outgoingX = outgoing.x
        savedIncoming = { x: incoming.x, opacity: incoming.opacity, visible: incoming.visible }
        incoming.visible = true
        incoming.opacity = 1
        updatePages()
    }

    function updatePages() {
        if (!tracking)
            return
        outgoing.x = outgoingX + progress * rootID.stackView.width
        incoming.x = -rootID.stackView.width * 0.25 * (1 - progress)
    }

    function restore() {
        settleID.stop()
        if (!tracking)
            return
        outgoing.x = outgoingX
        incoming.x = savedIncoming.x
        incoming.opacity = savedIncoming.opacity
        incoming.visible = savedIncoming.visible
        outgoing = null
        incoming = null
        savedIncoming = null
        progress = 0
    }

    function finish(commit) {
        if (!tracking || settleID.running)
            return
        completing = commit
        settleID.to = commit ? 1 : 0
        settleID.start()
    }

    implicitWidth: 24
    z: 2

    onProgressChanged: updatePages()
    onEnabledChanged: { if (!enabled) restore() }
    onWidthChanged: restore()

    NumberAnimation {
        id: settleID
        target: rootID
        property: "progress"
        duration: 220
        easing.type: Easing.OutCubic
        onFinished: {
            const commit = rootID.completing && rootID.stackView.currentItem === rootID.outgoing
            rootID.restore()
            if (commit)
                rootID.stackView.pop(StackView.Immediate)
        }
    }

    Connections {
        target: rootID.stackView
        function onCurrentItemChanged() { rootID.restore() }
        function onWidthChanged() { rootID.restore() }
    }

    Connections {
        target: Qt.application
        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive)
                rootID.restore()
        }
    }

    DragHandler {
        id: backDragID

        target: null
        enabled: !settleID.running
        maximumPointCount: 1
        yAxis.enabled: false

        onActiveChanged: {
            if (active)
                rootID.begin()
        }
        onTranslationChanged: {
            if (active && rootID.tracking) {
                const dx = centroid.position.x - centroid.pressPosition.x
                rootID.progress = Math.max(0, Math.min(1, dx / rootID.stackView.width))
            }
        }
        onGrabChanged: (transition, point) => {
            if (transition === PointerDevice.CancelGrabExclusive) {
                rootID.finish(false)
            } else if (transition === PointerDevice.UngrabExclusive && rootID.tracking) {
                const dx = point.position.x - point.pressPosition.x
                const dy = point.position.y - point.pressPosition.y
                const forwardFlick = point.velocity.x > 700 && dx > 30
                rootID.finish(true
                            && dx > Math.abs(dy) * 1.5
                            && (rootID.progress > 0.4 || forwardFlick))
            }
        }
    }
}