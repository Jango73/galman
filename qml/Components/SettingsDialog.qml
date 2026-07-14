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
        }
    }
}
