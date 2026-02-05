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
    title: action === "trash"
        ? qsTr("Move to trash?")
        : action === "delete"
            ? qsTr("Permanently delete?")
            : action === "move"
                ? qsTr("Move files?")
                : qsTr("Copy files?")
    property var sourcePane: null
    property var targetPane: null
    property int itemCount: 0
    property int fileCount: 0
    property int dirCount: 0
    property int nameConflictCount: 0
    property string directionText: ""
    property string action: "copy"
    signal copyConfirmed(var sourcePane, var targetPane)
    signal moveConfirmed(var sourcePane, var targetPane)
    signal trashConfirmed(var sourcePane)
    signal deleteConfirmed(var sourcePane)
    x: Math.round(((dialog.parent ? dialog.parent.width : 0) - width) / 2)
    y: Math.round(((dialog.parent ? dialog.parent.height : 0) - height) / 2)
    focus: true
    onOpened: forceActiveFocus()

    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: dialog.visible
        context: Qt.WindowShortcut
        onActivated: dialog.accept()
    }

    onAccepted: {
        if (action === "trash") {
            dialog.trashConfirmed(sourcePane)
            return
        }
        if (action === "delete") {
            dialog.deleteConfirmed(sourcePane)
            return
        }
        if (action === "move") {
            dialog.moveConfirmed(sourcePane, targetPane)
            return
        }
        dialog.copyConfirmed(sourcePane, targetPane)
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
                visible: dialog.action === "copy" || dialog.action === "move"
                text: (dialog.action === "move"
                    ? qsTr("%1, %2 will be moved (%3).")
                    : qsTr("%1, %2 will be copied (%3)."))
                    .arg(qsTr("%n file", "", dialog.fileCount))
                    .arg(qsTr("%n folder", "", dialog.dirCount))
                    .arg(dialog.directionText)
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: (dialog.action === "copy" || dialog.action === "move") && dialog.nameConflictCount > 0
                text: qsTr("Warning: %1 already exist in target.")
                    .arg(qsTr("%n item", "", dialog.nameConflictCount))
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: dialog.action === "copy" || dialog.action === "move"
                text: qsTr("Target: %1").arg(dialog.targetPane ? dialog.targetPane.currentPath : "")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: dialog.action === "trash"
                text: qsTr("%1 will be moved to the trash.").arg(qsTr("%n item", "", dialog.itemCount))
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                visible: dialog.action === "delete"
                text: qsTr("%1 will be permanently deleted.").arg(qsTr("%n item", "", dialog.itemCount))
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
