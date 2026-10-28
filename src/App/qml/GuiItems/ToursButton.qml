import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "../Helpers/colors.js" as Colors

Rectangle {
    id: rootID

    Accessible.role: Accessible.Button
    Accessible.name: "Tours"
    Accessible.onPressAction: clicked()

    signal clicked()

    width: 40
    height: 40
    radius: 10
    color: Colors.palette.accent

    Canvas {
        id: mapIconID

        anchors.centerIn: parent

        width: 24
        height: 24

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.strokeStyle = Colors.palette.bg
            ctx.fillStyle = Colors.palette.bg
            ctx.lineWidth = 1.5
            ctx.lineJoin = "round"
            ctx.lineCap = "round"

            // Three folded panels.
            ctx.beginPath()
            ctx.moveTo(2, 5)
            ctx.lineTo(8, 2)
            ctx.lineTo(15, 5)
            ctx.lineTo(22, 2)
            ctx.lineTo(22, 19)
            ctx.lineTo(15, 22)
            ctx.lineTo(8, 19)
            ctx.lineTo(2, 22)
            ctx.closePath()
            ctx.moveTo(8, 2)
            ctx.lineTo(8, 19)
            ctx.moveTo(15, 5)
            ctx.lineTo(15, 22)
            ctx.stroke()

            // Location pin on the right panel.
            ctx.beginPath()
            ctx.moveTo(18.5, 14)
            ctx.bezierCurveTo(17.5, 12.5, 16, 11, 16, 9.5)
            ctx.bezierCurveTo(16, 6.2, 21, 6.2, 21, 9.5)
            ctx.bezierCurveTo(21, 11, 19.5, 12.5, 18.5, 14)
            ctx.closePath()
            ctx.fill()
            ctx.fillStyle = Colors.palette.accent
            ctx.beginPath()
            ctx.arc(18.5, 9.5, 1, 0, 2 * Math.PI)
            ctx.fill()
        }

        Component.onCompleted: requestPaint()
    }

    TapHandler {
        onTapped: clicked()
    }
}
