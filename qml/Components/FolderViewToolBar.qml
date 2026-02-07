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
    property alias upButton: upButton
    property Item previousFocusItem: null
    property Item nextFocusItem: null
    property bool pathFieldActiveFocus: pathField.activeFocus
    signal goUpRequested()

    spacing: Theme.spaceSm

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

        ComboBox {
            id: volumeCombo
            model: root.volumeModel
            textRole: "label"
            Layout.preferredWidth: Theme.volumeComboPreferredWidth
            Layout.minimumWidth: Theme.volumeComboMinimumWidth
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

        TextField {
            id: pathField
            text: root.browserModel ? root.browserModel.rootPath : ""
            Layout.fillWidth: true
            KeyNavigation.backtab: volumeCombo
            KeyNavigation.tab: upButton
            onAccepted: {
                if (root.browserModel) {
                    root.browserModel.rootPath = text
                }
            }
        }

        Button {
            id: upButton
            text: "\u2190"
            KeyNavigation.backtab: pathField
            KeyNavigation.tab: root.nextFocusItem
            onClicked: {
                root.goUpRequested()
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
