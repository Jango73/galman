import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

import "."

Item {
    id: root
    property color panelBackground: Theme.panelBackground

    ComfyOutputBrowser {
        id: outputBrowser
        outputPath: comfyPilotController ? comfyPilotController.outputPath : ""
    }

    function previewExternalImage(path) {
        return outputBrowser.previewExternalImage(path)
    }

    onVisibleChanged: {
        if (visible) {
            outputPreview.forceActiveFocus()
        }
    }

    Connections {
        target: outputBrowser
        function onSelectionRestoredAfterRemoval() {
            outputPreview.forceActiveFocus()
        }
    }

    Connections {
        target: comfyPilotController
        function onRunningChanged() {
            if (!comfyPilotController || comfyPilotController.running) {
                return
            }
            if (comfyPilotController.outputPath === "") {
                return
            }
            outputBrowser.syncToOutput(comfyPilotController.outputPath)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spaceLg

        ComfyPilotControls {
            Layout.preferredWidth: parent ? parent.width * 0.25 : 0
            Layout.fillHeight: true
            panelBackground: root.panelBackground
            outputBrowser: outputBrowser
        }

        Pane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: Theme.panelPadding
            Material.background: root.panelBackground

            Item {
                anchors.fill: parent

                ImagePreview {
                    id: outputPreview
                    anchors.fill: parent
                    visible: outputBrowser.selectedMediaPath !== ""
                    panelBackground: root.panelBackground
                    browser: outputBrowser
                    onTrashConfirmationRequested: (count) => {
                        outputConfirmDialog.action = "trash"
                        outputConfirmDialog.sourcePane = outputPreview
                        outputConfirmDialog.targetPane = null
                        outputConfirmDialog.itemCount = count
                        outputConfirmDialog.directionText = ""
                        outputConfirmDialog.open()
                    }
                    onDeleteConfirmationRequested: (count) => {
                        outputConfirmDialog.action = "delete"
                        outputConfirmDialog.sourcePane = outputPreview
                        outputConfirmDialog.targetPane = null
                        outputConfirmDialog.itemCount = count
                        outputConfirmDialog.directionText = ""
                        outputConfirmDialog.open()
                    }
                    onSaveSucceeded: (message) => {
                        console.info(message)
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Theme.spaceSm
                    visible: outputBrowser.selectedMediaPath === ""
                        && !(comfyPilotController && comfyPilotController.running)

                    Label {
                        text: qsTr("No output yet")
                        opacity: 0.7
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Theme.spaceSm
                    visible: comfyPilotController && comfyPilotController.running

                    BusyIndicator {
                        running: true
                        width: 36
                        height: 36
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        text: qsTr("Generating...")
                        opacity: 0.8
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }
    }

    Shortcut {
        sequences: ["Delete"]
        context: Qt.WindowShortcut
        enabled: outputPreview.activeFocus && !outputConfirmDialog.visible
        onActivated: outputPreview.confirmTrashSelected()
    }

    Shortcut {
        sequences: ["Shift+Delete"]
        context: Qt.WindowShortcut
        enabled: outputPreview.activeFocus && !outputConfirmDialog.visible
        onActivated: outputPreview.confirmDeleteSelectedPermanently()
    }

    ConfirmDialog {
        id: outputConfirmDialog
        onTrashConfirmed: (sourcePane) => {
            if (sourcePane) {
                const result = sourcePane.trashSelected()
                if (result && result.error) {
                    console.warn(result.error)
                }
            }
        }
        onDeleteConfirmed: (sourcePane) => {
            if (sourcePane) {
                const result = sourcePane.deleteSelectedPermanently()
                if (result && result.error) {
                    console.warn(result.error)
                }
            }
        }
    }
}
