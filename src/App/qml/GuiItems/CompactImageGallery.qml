import QtQuick
import QtQuick.Controls

import "../Helpers/colors.js" as Colors

Item {
    id: rootID

    property var imageUrls: []
    readonly property int count: imagesViewID.count
    readonly property int currentIndex: imagesViewID.currentIndex

    implicitHeight: 180
    visible: count > 0

    Rectangle {
        anchors.fill: imagesViewID
        radius: 8
        color: Colors.palette.textEdit
    }

    ListView {
        id: imagesViewID
        objectName: "compactImagesView"

        anchors.fill: parent
        anchors.bottomMargin: dotsID.visible ? 20 : 0
        model: rootID.imageUrls
        orientation: ListView.Horizontal
        spacing: 8
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapOneItem
        highlightRangeMode: ListView.StrictlyEnforceRange
        preferredHighlightBegin: 0
        preferredHighlightEnd: 0
        highlightMoveDuration: 0

        delegate: Item {
            id: imageDelegateID
            required property var modelData

            width: imagesViewID.width
            height: imagesViewID.height

            Image {
                id: previewID
                anchors.fill: parent
                source: imageDelegateID.modelData
                asynchronous: true
                fillMode: Image.PreserveAspectFit
                sourceSize.width: Math.ceil(width * Screen.devicePixelRatio)
                sourceSize.height: Math.ceil(height * Screen.devicePixelRatio)
            }

            StyledBusyIndicator {
                anchors.centerIn: parent
                running: previewID.status === Image.Loading
                visible: running
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 24
                visible: previewID.status === Image.Error
                text: qsTr("Could not load image")
                color: Colors.palette.error
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }

    Label {
        anchors.top: imagesViewID.top
        anchors.right: imagesViewID.right
        anchors.margins: 6
        visible: rootID.count > 1
        text: qsTr("%1 / %2").arg(rootID.currentIndex + 1).arg(rootID.count)
        color: "white"
        font.bold: true
        padding: 5
        background: Rectangle {
            radius: 6
            color: Colors.palette.seeThroughBlack
        }
    }

    PageIndicator {
        id: dotsID
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        count: rootID.count
        currentIndex: rootID.currentIndex
        visible: count > 1 && count <= 7
        delegate: Rectangle {
            required property int index
            implicitWidth: 6
            implicitHeight: 6
            radius: 3
            color: index === rootID.currentIndex ? Colors.palette.accent : Colors.palette.toolbar
        }
    }
}
