import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

ColumnLayout {
    id: root
    property var browserModel: null
    property var volumeModel: null
    property bool volumeUpdating: false
    property alias pathField: pathField
    property alias volumeCombo: volumeCombo
    property alias goButton: goButton
    property alias upButton: upButton
    property alias refreshButton: refreshButton
    property Item previousFocusItem: null
    property Item nextFocusItem: null
    property bool pathFieldActiveFocus: pathField.activeFocus
    signal goUpRequested()

    spacing: Theme.spaceSm

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

        Label {
            text: qsTr("Volume")
            opacity: 0.7
        }

        ComboBox {
            id: volumeCombo
            model: root.volumeModel
            textRole: "label"
            Layout.fillWidth: true
            KeyNavigation.backtab: root.previousFocusItem
            KeyNavigation.tab: pathField
            onActivated: {
                if (!root.browserModel || root.volumeUpdating) {
                    return
                }
                const path = root.volumeModel ? root.volumeModel.pathForIndex(index) : ""
                if (path && path !== root.browserModel.rootPath) {
                    root.browserModel.rootPath = path
                }
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

        Label {
            text: qsTr("Path")
            opacity: 0.7
        }

        TextField {
            id: pathField
            text: root.browserModel ? root.browserModel.rootPath : ""
            Layout.fillWidth: true
            KeyNavigation.backtab: volumeCombo
            KeyNavigation.tab: goButton
            onAccepted: {
                if (root.browserModel) {
                    root.browserModel.rootPath = text
                }
            }
        }

        Button {
            id: goButton
            text: qsTr("Go")
            KeyNavigation.backtab: pathField
            KeyNavigation.tab: upButton
            onClicked: {
                if (root.browserModel) {
                    root.browserModel.rootPath = pathField.text
                }
            }
        }

        Button {
            id: upButton
            text: qsTr("Up")
            KeyNavigation.backtab: goButton
            KeyNavigation.tab: refreshButton
            onClicked: {
                root.goUpRequested()
            }
        }

        Button {
            id: refreshButton
            text: qsTr("Refresh")
            KeyNavigation.backtab: upButton
            KeyNavigation.tab: root.nextFocusItem
            onClicked: {
                if (root.browserModel) {
                    root.browserModel.refresh()
                }
            }
        }

        BusyIndicator {
            running: root.browserModel ? root.browserModel.loading : false
            visible: running
            width: 24
            height: 24
        }
    }
}
