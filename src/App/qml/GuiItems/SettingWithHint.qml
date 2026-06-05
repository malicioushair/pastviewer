import QtQuick
import QtQuick.Layouts

RowLayout {
    id: rootID

    Layout.fillWidth: true
    spacing: 8

    property string description: ""

    default property alias contents: contentHost.data

    Item {
        id: contentHost

        implicitHeight: childrenRect.height
        implicitWidth: childrenRect.width
    }

    SettingHelpHint {
        Layout.alignment: Qt.AlignRight
        description: rootID.description
        visible: rootID.description.length > 0
    }
}
