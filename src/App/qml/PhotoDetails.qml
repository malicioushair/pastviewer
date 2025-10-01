import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: rootID

    required property string imageSource
    required property int year

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            ToolButton {
                text: "←"
                onClicked: rootID.StackView.view.pop()
            }
            Label {
                Layout.fillWidth: true
                text: rootID.title
                wrapMode: Text.Wrap
            }
        }
    }

    footer: ToolBar {
        ColumnLayout {
            Label {
                Layout.leftMargin: 10
                text: qsTr("Year: ") + rootID.year
                font {
                    bold: true
                    pixelSize: 16
                }
                wrapMode: Text.WordWrap
            }
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 10
        }

        Item {
            id: viewportID

            Layout.fillWidth: true
            Layout.fillHeight: true

            clip: true

            Item {
                id: canvasID

                width: viewportID.width
                height: viewportID.height

                transformOrigin: Item.Center

                PinchHandler {
                    id: pinchHandlerID

                    target: null

                    rotationAxis.enabled: false
                    scaleAxis.minimum: 1.0
                    xAxis.enabled: false
                    yAxis.enabled: false
                    dragThreshold: 40

                    onScaleChanged: (delta) => {
                        canvasID.scale = canvasID.scale * delta < 1.0 ? 1.0 : canvasID.scale * delta
                    }

                    onActiveChanged: canvasID.anchors.centerIn = !active && imageID.fitsViewPort ? viewportID : undefined
                }
                DragHandler {
                    id: dragHandlerID

                    target: null

                    xAxis.onActiveValueChanged: (delta) => {
                        if (!imageID.fitsViewPort)
                            canvasID.x += delta
                    }
                    yAxis.onActiveValueChanged: (delta) => {
                        if (!imageID.fitsViewPort)
                            canvasID.y += delta
                    }
                }

                Image {
                    id: imageID

                    readonly property bool fitsViewPort: true
                            && imageID.status == Image.Ready
                            && imageID.paintedWidth * pinchHandlerID.scaleAxis.activeValue <= canvasID.width
                            && imageID.paintedHeight * pinchHandlerID.scaleAxis.activeValue <= canvasID.height

                    anchors.fill: parent
                    anchors.centerIn: parent

                    source: rootID.imageSource
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    antialiasing: true
                }
            }
        }
    }
}