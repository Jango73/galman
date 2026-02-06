import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

ColumnLayout {
    id: root
    property var browserModel: null
    property alias sortCombo: sortCombo
    property alias sortOrderBox: sortOrderBox
    property alias dirsFirstBox: dirsFirstBox
    property alias nameFilterField: nameFilterField
    property alias minimumByteSizeField: minimumByteSizeField
    property alias maximumByteSizeField: maximumByteSizeField
    property alias minimumImageWidthField: minimumImageWidthField
    property alias maximumImageWidthField: maximumImageWidthField
    property alias minimumImageHeightField: minimumImageHeightField
    property alias maximumImageHeightField: maximumImageHeightField
    property alias clearButton: clearFiltersButton
    property Item previousFocusItem: null
    property Item nextFocusItem: null
    property bool filterSelectByMouseEnabled: true
    property bool filterFieldActiveFocus: nameFilterField.activeFocus
        || minimumByteSizeField.activeFocus
        || maximumByteSizeField.activeFocus
        || minimumImageWidthField.activeFocus
        || maximumImageWidthField.activeFocus
        || minimumImageHeightField.activeFocus
        || maximumImageHeightField.activeFocus
    signal sortKeyChangedByUser(int sortKey)
    signal sortOrderChangedByUser(int sortOrder)
    signal dirsFirstChangedByUser(bool enabled)
    signal nameFilterChangedByUser(string filter)
    signal minimumByteSizeChangedByUser(int value)
    signal maximumByteSizeChangedByUser(int value)
    signal minimumImageWidthChangedByUser(int value)
    signal maximumImageWidthChangedByUser(int value)
    signal minimumImageHeightChangedByUser(int value)
    signal maximumImageHeightChangedByUser(int value)
    property var sortOptions: buildSortOptions()

    spacing: Theme.spaceSm

    function buildSortOptions() {
        return [qsTr("Name"), qsTr("Extension"), qsTr("Created"), qsTr("Modified"), qsTr("Signature")]
    }

    function parseNumberFilter(text) {
        const trimmed = String(text || "").trim()
        if (trimmed === "") {
            return Theme.filterUnsetValue
        }
        const numberValue = Number(trimmed)
        if (Number.isNaN(numberValue)) {
            return Theme.filterUnsetValue
        }
        return Math.floor(numberValue)
    }

    function setBrowserFilterValue(setter, value) {
        if (root.browserModel && setter) {
            setter(value)
        }
    }

    function clearFilters() {
        nameFilterField.text = ""
        minimumByteSizeField.text = ""
        maximumByteSizeField.text = ""
        minimumImageWidthField.text = ""
        maximumImageWidthField.text = ""
        minimumImageHeightField.text = ""
        maximumImageHeightField.text = ""
    }

    Connections {
        target: languageManager
        function onCurrentLanguageChanged() {
            root.sortOptions = root.buildSortOptions()
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

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
            onToggled: (checked) => {
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
            KeyNavigation.tab: nameFilterField
            onToggled: (checked) => {
                if (root.browserModel) {
                    root.browserModel.showDirsFirst = checked
                }
                root.dirsFirstChangedByUser(checked)
            }
        }

        Item { Layout.fillWidth: true }
    }

    ScrollView {
        id: filterScroll
        Layout.fillWidth: true
        ScrollBar.horizontal.policy: ScrollBar.AsNeeded
        ScrollBar.vertical.policy: ScrollBar.AlwaysOff
        clip: true
        contentWidth: filterRow.implicitWidth
        contentHeight: filterRow.implicitHeight

        RowLayout {
            id: filterRow
            spacing: Theme.spaceMd

            Label {
                text: qsTr("Filters")
            }

            TextField {
            id: nameFilterField
            placeholderText: qsTr("Name")
            Layout.preferredWidth: Theme.filterNameFieldWidth
            selectByMouse: root.filterSelectByMouseEnabled
            KeyNavigation.backtab: dirsFirstBox
            KeyNavigation.tab: minimumByteSizeField
            onTextChanged: {
                if (root.browserModel) {
                    root.browserModel.nameFilter = nameFilterField.text
                }
                root.nameFilterChangedByUser(nameFilterField.text)
            }
        }

            TextField {
            id: minimumByteSizeField
            visible: false
            placeholderText: qsTr("Min. bytes")
            Layout.preferredWidth: Theme.filterNumberFieldWidth
            selectByMouse: root.filterSelectByMouseEnabled
            KeyNavigation.backtab: nameFilterField
            KeyNavigation.tab: maximumByteSizeField
            inputMethodHints: Qt.ImhDigitsOnly
            onTextChanged: {
                const value = parseNumberFilter(minimumByteSizeField.text)
                setBrowserFilterValue((valueToSet) => { root.browserModel.minimumByteSize = valueToSet }, value)
                root.minimumByteSizeChangedByUser(value)
            }
        }

            TextField {
            id: maximumByteSizeField
            visible: false
            placeholderText: qsTr("Max. bytes")
            Layout.preferredWidth: Theme.filterNumberFieldWidth
            selectByMouse: root.filterSelectByMouseEnabled
            KeyNavigation.backtab: minimumByteSizeField
            KeyNavigation.tab: minimumImageWidthField
            inputMethodHints: Qt.ImhDigitsOnly
            onTextChanged: {
                const value = parseNumberFilter(maximumByteSizeField.text)
                setBrowserFilterValue((valueToSet) => { root.browserModel.maximumByteSize = valueToSet }, value)
                root.maximumByteSizeChangedByUser(value)
            }
        }

            TextField {
            id: minimumImageWidthField
            placeholderText: qsTr("Min. width")
            Layout.preferredWidth: Theme.filterNumberFieldWidth
            selectByMouse: root.filterSelectByMouseEnabled
            KeyNavigation.backtab: maximumByteSizeField
            KeyNavigation.tab: maximumImageWidthField
            inputMethodHints: Qt.ImhDigitsOnly
            onTextChanged: {
                const value = parseNumberFilter(minimumImageWidthField.text)
                setBrowserFilterValue((valueToSet) => { root.browserModel.minimumImageWidth = valueToSet }, value)
                root.minimumImageWidthChangedByUser(value)
            }
        }

            TextField {
            id: maximumImageWidthField
            placeholderText: qsTr("Max. width")
            Layout.preferredWidth: Theme.filterNumberFieldWidth
            selectByMouse: root.filterSelectByMouseEnabled
            KeyNavigation.backtab: minimumImageWidthField
            KeyNavigation.tab: minimumImageHeightField
            inputMethodHints: Qt.ImhDigitsOnly
            onTextChanged: {
                const value = parseNumberFilter(maximumImageWidthField.text)
                setBrowserFilterValue((valueToSet) => { root.browserModel.maximumImageWidth = valueToSet }, value)
                root.maximumImageWidthChangedByUser(value)
            }
        }

            TextField {
            id: minimumImageHeightField
            placeholderText: qsTr("Min. height")
            Layout.preferredWidth: Theme.filterNumberFieldWidth
            selectByMouse: root.filterSelectByMouseEnabled
            KeyNavigation.backtab: maximumImageWidthField
            KeyNavigation.tab: maximumImageHeightField
            inputMethodHints: Qt.ImhDigitsOnly
            onTextChanged: {
                const value = parseNumberFilter(minimumImageHeightField.text)
                setBrowserFilterValue((valueToSet) => { root.browserModel.minimumImageHeight = valueToSet }, value)
                root.minimumImageHeightChangedByUser(value)
            }
        }

            TextField {
            id: maximumImageHeightField
            placeholderText: qsTr("Max. height")
            Layout.preferredWidth: Theme.filterNumberFieldWidth
            selectByMouse: root.filterSelectByMouseEnabled
            KeyNavigation.backtab: minimumImageHeightField
            KeyNavigation.tab: clearFiltersButton
            inputMethodHints: Qt.ImhDigitsOnly
            onTextChanged: {
                const value = parseNumberFilter(maximumImageHeightField.text)
                setBrowserFilterValue((valueToSet) => { root.browserModel.maximumImageHeight = valueToSet }, value)
                root.maximumImageHeightChangedByUser(value)
            }
        }

            ToolButton {
                id: clearFiltersButton
                text: qsTr("Clear")
                KeyNavigation.backtab: maximumImageHeightField
                KeyNavigation.tab: root.nextFocusItem
                onClicked: clearFilters()
            }
        }
    }
}
