// StyledBusyIndicator.qml
import QtQuick
import QtQuick.Controls

import "../Helpers/colors.js" as Colors

BusyIndicator {
    id: control

    implicitWidth: 48
    implicitHeight: 48
    padding: 4

    contentItem: Canvas {
        id: canvas

        property real startAngle: -90
        property real endAngle: -90

        opacity: control.running ? 1 : 0

        onStartAngleChanged: requestPaint()
        onEndAngleChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const lineWidth = 4
            const radius = Math.min(width, height) / 2 - lineWidth / 2
            if (radius <= 0)
                return

            const cx = width / 2
            const cy = height / 2

            // Background ring
            ctx.lineWidth = lineWidth
            ctx.strokeStyle = Colors.palette.toolbar
            ctx.beginPath()
            ctx.arc(cx, cy, radius, 0, 2 * Math.PI)
            ctx.stroke()

            // Animated arc
            if (endAngle > startAngle) {
                ctx.strokeStyle = Colors.palette.accent
                ctx.lineCap = "round"
                ctx.beginPath()
                ctx.arc(
                    cx, cy, radius,
                    startAngle * Math.PI / 180,
                    endAngle * Math.PI / 180
                )
                ctx.stroke()
            }
        }

        SequentialAnimation {
            running: control.running && control.visible
            loops: Animation.Infinite

            // Begin with an empty ring.
            ScriptAction {
                script: {
                    canvas.startAngle = -90
                    canvas.endAngle = -90
                }
            }

            // Fill clockwise.
            NumberAnimation {
                target: canvas
                property: "endAngle"
                from: -90
                to: 270
                duration: 900
                easing.type: Easing.InOutSine
            }

            PauseAnimation { duration: 150 }

            // Empty clockwise.
            NumberAnimation {
                target: canvas
                property: "startAngle"
                from: -90
                to: 270
                duration: 900
                easing.type: Easing.InOutSine
            }

            PauseAnimation { duration: 150 }
        }
    }
}