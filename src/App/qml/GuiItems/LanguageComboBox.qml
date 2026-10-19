import QtQuick
import QtQuick.Controls

import "../Helpers/colors.js" as Colors

import PastViewer

ComboBox {
    id: controlID

    function setLanguage(language) {
        const value = language
        const index = controlID.indexOfValue(value)
        controlID.currentIndex = index
    }

    model: I18nController.languageModel
    textRole: "NameRole"
    valueRole: "CodeRole"
    currentIndex: {
        const index = I18nController.GetIndexOf(I18nController.GetCurrentLanguage())
        if (index >= 0)
            return index
    }

    background: Rectangle {
        color: Colors.palette.bg
        border {
            color: Colors.palette.border
            width: 1
        }
        radius: 8
    }

    contentItem: Text {
        text: controlID.displayText
        color: Colors.palette.text
        font.pixelSize: 14
        leftPadding: 12
        verticalAlignment: Text.AlignVCenter
    }

    popup: Popup {
        y: controlID.height
        width: controlID.width
        implicitHeight: contentItem.implicitHeight
        padding: 1

        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: controlID.popup.visible ? controlID.delegateModel : null
            currentIndex: controlID.highlightedIndex

            ScrollIndicator.vertical: ScrollIndicator { }
        }

        background: Rectangle {
            color: Colors.palette.bg
            border {
                color: Colors.palette.border
                width: 1
            }
            radius: 8
        }
    }

    delegate: ItemDelegate {
        width: controlID.width
        text: model.NameRole

        background: Rectangle {
            color: parent.hovered ? Colors.palette.accentAlt : Colors.palette.bg
            radius: 4
        }

        contentItem: Text {
            text: model.NameRole
            color: Colors.palette.text
            font.pixelSize: 14
            leftPadding: 12
            verticalAlignment: Text.AlignVCenter
        }
    }

    Component.onCompleted: {
        controlID.setLanguage(I18nController.GetCurrentLanguage())
    }

    Connections {
        target: I18nController
        function onCurrentLanguageChanged() {
            controlID.setLanguage(I18nController.GetCurrentLanguage())
        }
    }
}
