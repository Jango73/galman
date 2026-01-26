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
    title: qsTr("Rename")
    property string sourcePath: ""
    property string currentName: ""
    property string proposedName: ""
    property string errorText: ""
    signal renameConfirmed(string path, string newName)
    x: Math.round(((dialog.parent ? dialog.parent.width : 0) - width) / 2)
    y: Math.round(((dialog.parent ? dialog.parent.height : 0) - height) / 2)
    focus: true
    onOpened: {
        const nextName = proposedName !== "" ? proposedName : currentName
        nameField.text = nextName
        proposedName = ""
        nameField.forceActiveFocus()
        nameField.selectAll()
    }

    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: dialog.visible
        context: Qt.WindowShortcut
        onActivated: dialog.accept()
    }

    onAccepted: {
        dialog.renameConfirmed(sourcePath, nameField.text)
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
                text: qsTr("Current: %1").arg(dialog.currentName)
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            TextField {
                id: nameField
                text: ""
                placeholderText: qsTr("New name")
                Layout.fillWidth: true
            }

            Label {
                text: dialog.errorText
                visible: dialog.errorText !== ""
                color: Theme.statusDifferent
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
