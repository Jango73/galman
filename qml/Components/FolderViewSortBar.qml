import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

RowLayout {
    id: root
    property var browserModel: null
    property alias sortCombo: sortCombo
    property alias sortOrderBox: sortOrderBox
    property alias dirsFirstBox: dirsFirstBox
    property alias filterField: filterField
    property alias clearButton: clearButton
    property Item previousFocusItem: null
    property Item nextFocusItem: null
    property bool filterFieldActiveFocus: filterField.activeFocus
    signal sortKeyChangedByUser(int sortKey)
    signal sortOrderChangedByUser(int sortOrder)
    signal dirsFirstChangedByUser(bool enabled)
    signal nameFilterChangedByUser(string filter)
    property var sortOptions: buildSortOptions()

    spacing: Theme.spaceMd

    function buildSortOptions() {
        return [qsTr("Name"), qsTr("Extension"), qsTr("Created"), qsTr("Modified")]
    }

    Connections {
        target: languageManager
        function onCurrentLanguageChanged() {
            root.sortOptions = root.buildSortOptions()
        }
    }

    Label {
        text: qsTr("Sort by")
    }

    ComboBox {
        id: sortCombo
        model: root.sortOptions
        KeyNavigation.backtab: root.previousFocusItem
        KeyNavigation.tab: sortOrderBox
        onCurrentIndexChanged: {
            if (root.browserModel) {
                root.browserModel.sortKey = currentIndex
            }
            root.sortKeyChangedByUser(currentIndex)
        }
    }

    CheckBox {
        id: sortOrderBox
        text: qsTr("Descending")
        KeyNavigation.backtab: sortCombo
        KeyNavigation.tab: dirsFirstBox
        onToggled: {
            if (root.browserModel) {
                root.browserModel.sortOrder = checked ? Qt.DescendingOrder : Qt.AscendingOrder
            }
            root.sortOrderChangedByUser(checked ? Qt.DescendingOrder : Qt.AscendingOrder)
        }
    }

    CheckBox {
        id: dirsFirstBox
        text: qsTr("Folders first")
        KeyNavigation.backtab: sortOrderBox
        KeyNavigation.tab: filterField
        onToggled: {
            if (root.browserModel) {
                root.browserModel.showDirsFirst = checked
            }
            root.dirsFirstChangedByUser(checked)
        }
    }

    Item { Layout.fillWidth: true }

    TextField {
        id: filterField
        placeholderText: qsTr("Filter name...")
        Layout.fillWidth: true
        KeyNavigation.backtab: dirsFirstBox
        KeyNavigation.tab: clearButton
        onTextChanged: {
            if (root.browserModel) {
                root.browserModel.nameFilter = text
            }
            root.nameFilterChangedByUser(text)
        }
    }

    ToolButton {
        id: clearButton
        text: qsTr("X")
        KeyNavigation.backtab: filterField
        KeyNavigation.tab: root.nextFocusItem
        onClicked: filterField.text = ""
    }
}
