import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

Dialog {
    id: dialog
    modal: true
    title: qsTr("About")
    standardButtons: Dialog.Ok
    Overlay.modal: Rectangle {
        color: Theme.modalOverlayColor
    }
    focus: true
    width: Math.min(520, dialog.parent ? Math.round(dialog.parent.width * 0.75) : 520)
    x: Math.round(((dialog.parent ? dialog.parent.width : 0) - width) / 2)
    y: Math.round(((dialog.parent ? dialog.parent.height : 0) - height) / 2)
    onOpened: forceActiveFocus()

    property string applicationName: "Galman"
    property string applicationVersion: ""
    property string applicationDescription: qsTr("Image gallery manager for fast browsing, comparison, and file operations.")

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        Label {
            text: dialog.applicationName
            font.pixelSize: Theme.fontSizeTitle
            font.bold: true
            Layout.fillWidth: true
        }

        Label {
            text: qsTr("Version: %1").arg(dialog.applicationVersion !== "" ? dialog.applicationVersion : qsTr("unknown"))
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        Label {
            text: dialog.applicationDescription
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }
    }
}
