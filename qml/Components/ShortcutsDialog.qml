import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

Dialog {
    id: dialog
    modal: true
    title: qsTr("Shortcuts")
    standardButtons: Dialog.Ok
    Overlay.modal: Rectangle {
        color: Theme.modalOverlayColor
    }
    focus: true
    width: Math.min(620, dialog.parent ? Math.round(dialog.parent.width * 0.8) : 620)
    height: Math.min(560, dialog.parent ? Math.round(dialog.parent.height * 0.8) : 560)
    x: Math.round(((dialog.parent ? dialog.parent.width : 0) - width) / 2)
    y: Math.round(((dialog.parent ? dialog.parent.height : 0) - height) / 2)
    onOpened: forceActiveFocus()

    property var sections: buildSections()

    function buildSections() {
        return [
            {
                title: qsTr("General"),
                items: [
                    { keys: "Backspace", action: qsTr("Go up folder (focused pane)") },
                    { keys: "Ctrl+S", action: qsTr("Toggle synchronize mode") },
                    { keys: "Ctrl+H", action: qsTr("Toggle hide identical") },
                    { keys: "Ctrl+Enter / Ctrl+Return", action: qsTr("Open item externally") }
                ]
            },
            {
                title: qsTr("Folder View"),
                items: [
                    { keys: "Del", action: qsTr("Move selection to trash (asks for confirmation)") },
                    { keys: "F5", action: qsTr("Refresh current folder") },
                    { keys: "Ctrl+Left", action: qsTr("Copy selection right to left") },
                    { keys: "Ctrl+Right", action: qsTr("Copy selection left to right") },
                    { keys: "Arrows", action: qsTr("Move current item") },
                    { keys: "Shift+Arrows / Shift+Home / Shift+End", action: qsTr("Extend selection range") },
                    { keys: "Ctrl+Up / Ctrl+Down / Ctrl+Home / Ctrl+End", action: qsTr("Move current item without changing selection") },
                    { keys: "Home / End", action: qsTr("Go to first / last item") },
                    { keys: "Enter / Return", action: qsTr("Open item internally") },
                    { keys: "Ctrl+A", action: qsTr("Select all (or clear selection if all selected)") },
                    { keys: "Space", action: qsTr("Select current item") },
                    { keys: "Ctrl+Space", action: qsTr("Toggle current item selection") },
                    { keys: "Tab / Shift+Tab", action: qsTr("Move focus to toolbar fields") }
                ]
            },
            {
                title: qsTr("Preview"),
                items: [
                    { keys: "Esc", action: qsTr("Close preview") },
                    { keys: "Left / Right", action: qsTr("Previous / next image") },
                    { keys: "Home / End", action: qsTr("First / last image") },
                    { keys: "Ctrl+Left", action: qsTr("Copy selection right to left") },
                    { keys: "Ctrl+Right", action: qsTr("Copy selection left to right") }
                ]
            }
        ]
    }

    Connections {
        target: languageManager
        function onCurrentLanguageChanged() {
            dialog.sections = dialog.buildSections()
        }
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: scrollView.availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: Theme.spaceMd

            Repeater {
                model: dialog.sections
                delegate: ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    Label {
                        text: modelData.title
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: modelData.items
                        delegate: Label {
                            text: modelData.keys + " - " + modelData.action
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
