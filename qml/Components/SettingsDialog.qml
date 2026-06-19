import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

Dialog {
    id: dialog
    modal: true
    title: qsTr("Settings")
    standardButtons: Dialog.Ok | Dialog.Cancel
    Material.theme: Material.Dark
    Material.primary: Material.Blue
    Material.accent: Material.DeepOrange
    Overlay.modal: Rectangle {
        color: Theme.modalOverlayColor
    }
    focus: true
    width: Math.min(520, dialog.parent ? Math.round(dialog.parent.width * 0.75) : 520)
    x: Math.round(((dialog.parent ? dialog.parent.width : 0) - width) / 2)
    y: Math.round(((dialog.parent ? dialog.parent.height : 0) - height) / 2)

    property string junkExtensionsInput: junkField.text

    function setInitialJunkText(text) {
        junkField.text = text
    }

    onOpened: {
        junkField.forceActiveFocus()
        junkField.selectAll()
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        Label {
            text: qsTr("Junk file extensions")
            font.bold: true
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Files with these extensions will be hidden when \"Hide junk files\" is enabled. Use commas to separate multiple extensions.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        TextField {
            id: junkField
            placeholderText: qsTr(".jpg~,.png~,.blend1")
            Layout.fillWidth: true
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
