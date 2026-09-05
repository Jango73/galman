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
    width: dialog.parent ? Math.round(dialog.parent.width * 0.35) : 520
    height: dialog.parent ? Math.round(dialog.parent.height * 0.75) : 440
    x: Math.round(((dialog.parent ? dialog.parent.width : 0) - width) / 2)
    y: Math.round(((dialog.parent ? dialog.parent.height : 0) - height) / 2)

    property string junkExtensionsInput: junkField.text
    property int backupsLimitInput: backupsLimitSpinBox.value

    function setInitialJunkText(text) {
        junkField.text = text
    }

    function setInitialBackupsLimit(value) {
        backupsLimitSpinBox.value = value
    }

    onOpened: {
        tabBar.currentIndex = 0
        junkField.forceActiveFocus()
        junkField.selectAll()
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Material.foreground: Material.accent

            TabButton {
                text: qsTr("General")
            }

            TabButton {
                text: qsTr("Favorites")
            }

            TabButton {
                text: qsTr("ComfyUI")
            }
        }

        StackLayout {
            currentIndex: tabBar.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
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
                    height: Theme.spaceLg
                }

                Label {
                    text: qsTr("Backups")
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Maximum number of backup copies to keep per file. Older backups are removed when the limit is exceeded.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                SpinBox {
                    id: backupsLimitSpinBox
                    from: 1
                    to: 9999
                    value: 20
                    editable: true
                    Layout.preferredWidth: 120
                }

                Item {
                    Layout.fillHeight: true
                }
            }

            ColumnLayout {
                spacing: Theme.spaceMd

                Label {
                    text: qsTr("Saved favorite pairs")
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Reorder or remove saved left/right folder pairs.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: favoritesList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: favoritesManager ? favoritesManager.favoritePairs : []
                        currentIndex: -1

                        delegate: Rectangle {
                            width: favoritesList.width
                            height: delegateColumn.implicitHeight + Theme.spaceMd * 2
                            color: favoritesList.currentIndex === index ? Qt.rgba(Material.accent.r, Material.accent.g, Material.accent.b, 0.2) : "transparent"
                            border.color: favoritesList.currentIndex === index ? Material.accent : "transparent"
                            border.width: 1
                            radius: 2

                            ColumnLayout {
                                id: delegateColumn
                                anchors.fill: parent
                                anchors.margins: Theme.spaceSm
                                spacing: Theme.spaceXs

                                Label {
                                    text: modelData.leftPath
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                    font.pixelSize: 12
                                }

                                Label {
                                    text: modelData.rightPath
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                    font.pixelSize: 12
                                    opacity: 0.7
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: (mouse) => favoritesList.currentIndex = index
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }
                    }

                    ColumnLayout {
                        spacing: Theme.spaceSm

                        Button {
                            text: qsTr("Up")
                            icon.name: "go-up"
                            enabled: favoritesList.currentIndex > 0
                            onClicked: {
                                var idx = favoritesList.currentIndex
                                favoritesManager.moveFavoritePair(idx, idx - 1)
                                favoritesList.currentIndex = idx - 1
                            }
                        }

                        Button {
                            text: qsTr("Down")
                            icon.name: "go-down"
                            enabled: favoritesList.currentIndex >= 0 && favoritesList.currentIndex < favoritesList.count - 1
                            onClicked: {
                                var idx = favoritesList.currentIndex
                                favoritesManager.moveFavoritePair(idx, idx + 1)
                                favoritesList.currentIndex = idx + 1
                            }
                        }

                        Item { height: Theme.spaceLg }

                        Button {
                            text: qsTr("Remove")
                            icon.name: "edit-delete"
                            enabled: favoritesList.currentIndex >= 0
                            onClicked: {
                                var idx = favoritesList.currentIndex
                                favoritesManager.removeFavoritePair(idx)
                                if (favoritesList.count > 0) {
                                    favoritesList.currentIndex = Math.min(idx, favoritesList.count - 1)
                                } else {
                                    favoritesList.currentIndex = -1
                                }
                            }
                        }
                    }
                }
            }

            ScrollView {
                id: comfyScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: comfyScroll.availableWidth
                    spacing: Theme.spaceMd

                    Label {
                        text: qsTr("ComfyUI server")
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Address of the ComfyUI server used for image and video generation.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                TextField {
                    id: serverField
                    placeholderText: comfyPilotController ? comfyPilotController.defaultServerUrl : ""
                    text: comfyPilotController ? comfyPilotController.serverUrl : ""
                    Layout.fillWidth: true
                    onTextChanged: {
                        if (comfyPilotController) {
                            comfyPilotController.serverUrl = text
                        }
                    }
                }

                Label {
                    text: qsTr("Prerequisites")
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Nodes and models required for image and video generation. Check the statuses, then install anything missing from ComfyUI Manager.")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("Remote server configured: node statuses are checked on the server, but model files can only be checked locally.")
                    visible: comfyPrerequisites && !comfyPrerequisites.isLocalServer(serverField.text)
                    color: Theme.statusDifferent
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceSm

                    Button {
                        text: qsTr("Check nodes")
                        enabled: comfyPrerequisites && !comfyPrerequisites.checking
                        onClicked: {
                            comfyPrerequisites.refresh(serverField.text)
                        }
                    }

                    Button {
                        text: qsTr("Open ComfyUI Manager")
                        enabled: comfyPrerequisites !== null
                        onClicked: {
                            comfyPrerequisites.openManager(serverField.text)
                        }
                    }
                }

                Label {
                    text: comfyPrerequisites ? comfyPrerequisites.statusMessage : ""
                    visible: comfyPrerequisites && comfyPrerequisites.statusMessage !== ""
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    text: comfyPrerequisites ? comfyPrerequisites.errorMessage : ""
                    visible: comfyPrerequisites && comfyPrerequisites.errorMessage !== ""
                    color: Theme.danger
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Label {
                    text: prerequisiteSummary()
                    Layout.fillWidth: true
                    font.pixelSize: 12
                    opacity: 0.8
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    Repeater {
                        model: comfyPrerequisites ? comfyPrerequisites.nodeStatuses : []

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spaceSm

                            Rectangle {
                                width: 10
                                height: 10
                                radius: 5
                                color: prerequisiteStatusColor(modelData.status)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label {
                                    text: modelData.classType
                                    Layout.fillWidth: true
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }

                                Label {
                                    text: modelData.packageId + " - " + formatTargets(modelData.requiredFor)
                                    Layout.fillWidth: true
                                    font.pixelSize: 11
                                    opacity: 0.7
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }

                Label {
                    text: qsTr("Required models")
                    font.bold: true
                    Layout.fillWidth: true
                }

                Label {
                    text: prerequisiteModelSummary()
                    Layout.fillWidth: true
                    font.pixelSize: 12
                    opacity: 0.8
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spaceXs

                    Repeater {
                        model: comfyPrerequisites ? comfyPrerequisites.modelRequirements : []

                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.spaceSm

                            Rectangle {
                                width: 10
                                height: 10
                                radius: 5
                                color: prerequisiteStatusColor(modelData.status)
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Label {
                                    text: modelData.fileName
                                    Layout.fillWidth: true
                                    font.pixelSize: 12
                                    elide: Text.ElideMiddle
                                }

                                Label {
                                    text: "models/" + modelData.folder + " (" + formatTargets(modelData.requiredFor) + ")"
                                    Layout.fillWidth: true
                                    font.pixelSize: 11
                                    opacity: 0.7
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

        }
    }
    }

    function prerequisiteStatusColor(status) {
        if (!comfyPrerequisites) {
            return Theme.statusDifferent
        }
        if (status === comfyPrerequisites.statusInstalled) {
            return Theme.statusIdentical
        }
        if (status === comfyPrerequisites.statusMissing) {
            return Theme.statusMissing
        }
        return Theme.statusDifferent
    }

    function formatTargets(targets) {
        if (targets === undefined || targets === null || targets.length === undefined) {
            return ""
        }
        return Array.prototype.join.call(targets, ", ")
    }

    function countByStatus(entries) {
        var counts = {"installed": 0, "missing": 0}
        if (!comfyPrerequisites || entries === undefined || entries === null) {
            return counts
        }
        for (var i = 0; i < entries.length; i++) {
            if (entries[i].status === comfyPrerequisites.statusInstalled) {
                counts.installed++
            } else if (entries[i].status === comfyPrerequisites.statusMissing) {
                counts.missing++
            }
        }
        return counts
    }

    function prerequisiteSummary() {
        var counts = countByStatus(comfyPrerequisites ? comfyPrerequisites.nodeStatuses : null)
        if (counts.installed === 0 && counts.missing === 0) {
            return qsTr("Node statuses unknown - run a check.")
        }
        return qsTr("%1 installed, %2 missing").arg(counts.installed).arg(counts.missing)
    }

    function prerequisiteModelSummary() {
        var counts = countByStatus(comfyPrerequisites ? comfyPrerequisites.modelRequirements : null)
        if (counts.installed === 0 && counts.missing === 0) {
            return qsTr("Model statuses unknown - run a check.")
        }
        return qsTr("%1 installed, %2 missing").arg(counts.installed).arg(counts.missing)
    }
}
