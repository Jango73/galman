import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

Dialog {
    id: dialog
    modal: true
    Material.theme: Material.Dark
    Material.primary: Material.Blue
    Material.accent: Material.DeepOrange
    Overlay.modal: Rectangle {
        color: Theme.modalOverlayColor
    }
    standardButtons: Dialog.Ok | Dialog.Cancel

    property string titleText: ""
    property string description: ""
    property string placeholderText: ""
    property string currentText: ""
    property bool selectAllOnOpen: false
    signal textAccepted(string text)

    title: titleText

    x: Math.round(((dialog.parent ? dialog.parent.width : 0) - width) / 2)
    y: Math.round(((dialog.parent ? dialog.parent.height : 0) - height) / 2)
    focus: true

    onOpened: {
        nameField.text = currentText
        nameField.forceActiveFocus()
        if (selectAllOnOpen) {
            nameField.selectAll()
        }
    }

    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: dialog.visible
        context: Qt.WindowShortcut
        onActivated: dialog.accept()
    }

    onAccepted: {
        dialog.textAccepted(nameField.text)
    }

    contentItem: Item {
        id: contentRoot
        focus: true

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Escape) {
                dialog.reject()
                event.accepted = true
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spaceSm

            Label {
                text: dialog.description
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: dialog.description.length > 0
            }

            TextField {
                id: nameField
                text: ""
                placeholderText: dialog.placeholderText
                Layout.fillWidth: true
            }
        }
    }
}
