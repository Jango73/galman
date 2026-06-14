import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import "."
import Galman 1.0

FocusScope {
    id: root
    clip: true

    signal folderActivated(string path)
    signal fileActivated(string path)
    signal selectionChanged(var paths)
    signal viewSyncRequested()
    signal trashConfirmationRequested(int itemCount)
    signal trashFinished(var result)
    signal deleteConfirmationRequested(int itemCount)
    signal copyLeftRequested()
    signal copyRightRequested()
    signal copyOtherRequested()
    signal moveLeftRequested()
    signal moveRightRequested()
    signal moveOtherRequested()
    signal copyFinished(var result)
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
    signal goUpRequested()
    signal renameSucceeded(string message)
    signal errorRaised(string message)
    property string settingsKey: ""
    property bool useExternalModel: false
    property var externalModel: null
    readonly property var browserModel: (useExternalModel && externalModel) ? externalModel : defaultModel
    property string currentPath: browserModel ? browserModel.rootPath : ""
    property int selectionCount: browserModel ? browserModel.selectedPaths.length : 0
    property bool selectedIsImage: browserModel ? browserModel.selectedIsImage : false
    property bool selectedIsVideo: browserModel ? browserModel.selectedIsVideo : false
    property int selectedFileCount: browserModel ? browserModel.selectedFileCount : 0
    property real selectedTotalBytes: browserModel ? browserModel.selectedTotalBytes : 0
    property bool copyInProgress: browserModel ? browserModel.copyInProgress : false
    property real copyProgress: browserModel ? browserModel.copyProgress : 0
    property color panelBackground: Material.background
    property int lastCurrentIndex: -1
    property int restoreCurrentIndex: -1
    property int pendingDeletionIndex: -1
    property bool pendingDeletionFocus: false
    property string lastSelectedPath: ""
    property string restoreAdjacentPath: ""
    property bool restoringIndex: false
    property bool selectionAlignmentPending: false
    property bool pendingIndexReset: false
    property real pendingScrollOffset: -1
    property bool syncEnabled: true
    property bool rootPathSyncing: false
    property bool textInputActive: toolbar.pathFieldActiveFocus || sortBar.filterFieldActiveFocus
    property bool hasFocus: browserGrid.gridActiveFocus || textInputActive
    property Item previousPanelFocusItem: null
    property Item nextPanelFocusItem: null
    readonly property Item firstFocusItem: toolbar.volumeCombo
    readonly property Item lastFocusItem: browserGrid.focusItem
    property bool volumeUpdating: false
    property string selectedMediaPath: ""
    property int selectedCompareStatus: 0
    property bool selectedGhost: false
    readonly property bool pendingTrue: true
    readonly property int statusPending: 1
    readonly property int statusIdentical: 2
    readonly property int statusDifferent: 3
    readonly property int statusMissing: 4
    readonly property color statusIdenticalColor: Theme.statusIdentical
    readonly property color statusDifferentColor: Theme.statusDifferent
    readonly property color statusMissingColor: Theme.statusMissing

    FolderBrowserModel {
        id: defaultModel
        settingsKey: root.settingsKey
    }

    VolumeModel {
        id: volumeModel
    }

    Timer {
        id: selectionAlignmentTimer
        interval: 50
        repeat: false
        onTriggered: root.synchronizeCurrentIndexWithSelection()
    }

    onBrowserModelChanged: {
        if (browserModel) {
            browserModel.settingsKey = settingsKey
        }
        updateSelectedMediaPath()
    }

    onSettingsKeyChanged: {
        if (browserModel) {
            browserModel.settingsKey = settingsKey
        }
    }

    function copySelectionTo(targetPath) {
        if (!browserModel) {
            return {"ok": false, "error": "No model"};
        }
        return browserModel.copySelectedTo(targetPath)
    }

    function startCopySelectionTo(targetPath) {
        if (!browserModel || !browserModel.startCopySelectedTo) {
            return
        }
        browserModel.startCopySelectedTo(targetPath)
    }

    function startMoveSelectionTo(targetPath) {
        if (!browserModel || !browserModel.startMoveSelectedTo) {
            return
        }
        browserModel.startMoveSelectedTo(targetPath)
    }

    function cancelCopy() {
        if (!browserModel || !browserModel.cancelCopy) {
            return
        }
        browserModel.cancelCopy()
    }

    function selectedRows() {
        if (!browserModel || !browserModel.selectedRows) {
            return []
        }
        return browserModel.selectedRows()
    }

    function selectedPaths() {
        return browserModel ? browserModel.selectedPaths : []
    }

    function copyNameConflictCount(targetPath) {
        if (!browserModel || !browserModel.copyNameConflictCount) {
            return 0
        }
        return browserModel.copyNameConflictCount(targetPath)
    }

    function selectionStats() {
        if (browserModel && browserModel.selectionStats) {
            return browserModel.selectionStats()
        }
        return { "dirs": 0, "files": 0 }
    }

    function updateSelectedMediaPath() {
        if (!browserModel || (!browserModel.selectedIsImage && !browserModel.selectedIsVideo)) {
            selectedMediaPath = ""
            selectedCompareStatus = 0
            selectedGhost = false
            return
        }
        const paths = browserModel.selectedPaths || []
        selectedMediaPath = paths.length > 0 ? paths[0] : ""
        selectedCompareStatus = browserModel.selectedCompareStatus ? browserModel.selectedCompareStatus() : 0
        selectedGhost = browserModel.selectedIsGhost ? browserModel.selectedIsGhost() : false
    }

    function rememberCurrentPath() {
        if (restoringIndex) {
            return
        }
        let nextPath = ""
        if (browserModel && browserModel.pathForRow && lastCurrentIndex >= 0) {
            nextPath = browserModel.pathForRow(lastCurrentIndex)
        }
        if (!nextPath) {
            const selected = browserModel ? browserModel.selectedPaths : []
            nextPath = selected && selected.length > 0 ? selected[0] : ""
        }
        if (nextPath) {
            lastSelectedPath = nextPath
        }
    }

    function baseNameFromPath(path) {
        const normalized = String(path || "").replace(/\\/g, "/")
        const idx = normalized.lastIndexOf("/")
        return idx >= 0 ? normalized.slice(idx + 1) : normalized
    }

    function folderPathFromPath(path) {
        const normalized = String(path || "").replace(/\\/g, "/")
        const idx = normalized.lastIndexOf("/")
        return idx >= 0 ? normalized.slice(0, idx) : ""
    }

    function baseNameWithoutExtensionFromPath(path) {
        const filename = baseNameFromPath(path)
        const idx = filename.lastIndexOf(".")
        return idx > 0 ? filename.slice(0, idx) : filename
    }

    function rowForPath(path) {
        if (!browserGrid || !browserModel || !browserModel.pathForRow || !path) {
            return -1
        }
        const count = browserGrid.count
        for (let i = 0; i < count; i += 1) {
            if (browserModel.pathForRow(i) === path) {
                return i
            }
        }
        return -1
    }

    function adjacentPathForRow(row) {
        if (!browserGrid || !browserModel || !browserModel.pathForRow) {
            return ""
        }
        const count = browserGrid.count
        if (row < 0 || row >= count) {
            return ""
        }
        const nextRow = row + 1
        if (nextRow < count) {
            return browserModel.pathForRow(nextRow)
        }
        const previousRow = row - 1
        if (previousRow >= 0) {
            return browserModel.pathForRow(previousRow)
        }
        return ""
    }

    function findReplacementPath(targetPath) {
        if (!browserGrid || !browserModel || !browserModel.pathForRow || !targetPath) {
            return ""
        }
        const targetFolder = folderPathFromPath(targetPath)
        const targetBaseName = baseNameWithoutExtensionFromPath(targetPath)
        if (targetBaseName === "") {
            return ""
        }
        const count = browserGrid.count
        for (let i = 0; i < count; i += 1) {
            const candidatePath = browserModel.pathForRow(i)
            if (!candidatePath) {
                continue
            }
            if (folderPathFromPath(candidatePath) !== targetFolder) {
                continue
            }
            if (baseNameWithoutExtensionFromPath(candidatePath) === targetBaseName) {
                return candidatePath
            }
        }
        return ""
    }

    function findDirectoryPathByName(name) {
        if (!browserGrid || !browserModel || !browserModel.isDir || !browserModel.pathForRow) {
            return ""
        }
        const target = String(name || "")
        if (target === "") {
            return ""
        }
        const count = browserGrid.count
        for (let i = 0; i < count; i += 1) {
            if (!browserModel.isDir(i)) {
                continue
            }
            const path = browserModel.pathForRow(i)
            if (baseNameFromPath(path) === target) {
                return path
            }
        }
        return ""
    }

    function selectAdjacentMedia(step) {
        if (!browserModel || !browserGrid || browserGrid.count === 0) {
            return
        }
        let index = browserGrid.currentIndex
        if (browserModel.selectedRows) {
            const rows = browserModel.selectedRows()
            if (rows && rows.length > 0) {
                const firstSelected = rows[0]
                const currentIsSelected = rows.indexOf(index) >= 0
                if (!currentIsSelected) {
                    index = firstSelected
                }
            }
        }
        if (index < 0) {
            index = 0
        }
        const count = browserGrid.count
        for (let i = 0; i < count; i += 1) {
            index += step
            if (index < 0 || index >= count) {
                return
            }
            const itemIsImage = browserModel.isImage ? browserModel.isImage(index) : false
            const itemIsVideo = browserModel.isVideo ? browserModel.isVideo(index) : false
            if (!itemIsImage && !itemIsVideo) {
                continue
            }
            if (browserModel.isGhost && browserModel.isGhost(index)) {
                continue
            }
            browserModel.select(index, false)
            browserGrid.anchorIndex = index
            browserGrid.setCurrentIndex(index)
            browserGrid.positionViewAtIndex(index)
            return
        }
    }

    function selectBoundaryMedia(first) {
        if (!browserModel || !browserGrid || browserGrid.count === 0) {
            return
        }
        const count = browserGrid.count
        if (first) {
            for (let i = 0; i < count; i += 1) {
                const itemIsImage = browserModel.isImage ? browserModel.isImage(i) : false
                const itemIsVideo = browserModel.isVideo ? browserModel.isVideo(i) : false
                if (!itemIsImage && !itemIsVideo) {
                    continue
                }
                if (browserModel.isGhost && browserModel.isGhost(i)) {
                    continue
                }
                browserModel.select(i, false)
                browserGrid.anchorIndex = i
                browserGrid.setCurrentIndex(i)
                browserGrid.positionViewAtIndex(i)
                return
            }
        } else {
            for (let i = count - 1; i >= 0; i -= 1) {
                const itemIsImage = browserModel.isImage ? browserModel.isImage(i) : false
                const itemIsVideo = browserModel.isVideo ? browserModel.isVideo(i) : false
                if (!itemIsImage && !itemIsVideo) {
                    continue
                }
                if (browserModel.isGhost && browserModel.isGhost(i)) {
                    continue
                }
                browserModel.select(i, false)
                browserGrid.anchorIndex = i
                browserGrid.setCurrentIndex(i)
                browserGrid.positionViewAtIndex(i)
                return
            }
        }
    }

    function setSelectionRows(rows, additive) {
        if (!browserModel) {
            return
        }
        browserModel.setSelection(rows, additive)
    }

    function clearSelection() {
        if (browserModel) {
            browserModel.clearSelection()
        }
    }

    function goUp() {
        if (!browserModel || !browserModel.goUp) {
            return
        }
        browserModel.goUp()
        root.goUpRequested()
    }

    function setCurrentIndex(row) {
        if (!browserGrid || row < 0) {
            return
        }
        browserGrid.setCurrentIndex(row)
        browserGrid.positionViewAtIndex(row)
    }

    function setScrollOffset(offset) {
        if (!browserGrid) {
            return
        }
        browserGrid.setContentY(offset)
    }

    function scrollOffset() {
        return browserGrid ? browserGrid.contentY : 0
    }

    function currentIndex() {
        return browserGrid ? browserGrid.currentIndex : -1
    }

    function refresh() {
        if (browserModel) {
            browserModel.refresh()
        }
    }

    function refreshPaths(paths) {
        if (!browserModel) {
            return
        }
        if (browserModel.refreshFiles) {
            browserModel.refreshFiles(paths)
        } else {
            browserModel.refresh()
        }
    }

    function rememberScrollOffset() {
        if (browserGrid) {
            pendingScrollOffset = browserGrid.contentY
        }
    }

    function restoreScrollOffset() {
        if (!browserGrid || pendingScrollOffset < 0) {
            return
        }
        browserGrid.setContentY(pendingScrollOffset)
        pendingScrollOffset = -1
    }

    function clampIndex(index, count) {
        if (count <= 0) {
            return -1
        }
        if (index < 0) {
            return 0
        }
        if (index >= count) {
            return count - 1
        }
        return index
    }

    function applyRestore() {
        if (!browserGrid) {
            return
        }
        if (browserGrid.count === 0) {
            restoringIndex = false
            pendingDeletionIndex = -1
            restoreAdjacentPath = ""
            return
        }
        let nextIndex = -1
        const targetPath = pendingDeletionIndex >= 0 ? "" : lastSelectedPath
        if (targetPath !== "") {
            nextIndex = rowForPath(targetPath)
            if (nextIndex < 0) {
                const replacementPath = findReplacementPath(targetPath)
                if (replacementPath !== "") {
                    nextIndex = rowForPath(replacementPath)
                }
            }
        }
        if (nextIndex < 0 && browserModel && browserModel.selectedRows) {
            const rows = browserModel.selectedRows()
            if (rows && rows.length > 0) {
                nextIndex = rows[0]
            }
        }
        if (nextIndex < 0 && restoreAdjacentPath !== "") {
            nextIndex = rowForPath(restoreAdjacentPath)
        }
        if (nextIndex < 0) {
            nextIndex = clampIndex(restoreCurrentIndex, browserGrid.count)
        }
        if (nextIndex >= 0) {
            browserGrid.setCurrentIndex(nextIndex)
            if (browserModel && browserModel.select) {
                browserModel.select(nextIndex, false)
            }
            browserGrid.positionViewAtIndex(nextIndex)
        } else {
            browserGrid.setCurrentIndex(-1)
        }
        restoringIndex = false
        lastCurrentIndex = browserGrid.currentIndex
        rememberCurrentPath()
        updateSelectedMediaPath()
        pendingDeletionIndex = -1
        restoreAdjacentPath = ""
    }

    function alignCurrentIndexWithSelection() {
        if (!browserGrid || !browserModel || !browserModel.selectedPaths) {
            return
        }
        const paths = browserModel.selectedPaths
        if (!paths || paths.length !== 1) {
            return
        }
        const selectedPath = paths[0]
        const selectedRow = rowForPath(selectedPath)
        if (selectedRow < 0) {
            return
        }
        if (browserGrid.currentIndex === selectedRow) {
            return
        }
        browserGrid.setCurrentIndex(selectedRow)
        browserGrid.positionViewAtIndex(selectedRow)
        root.lastCurrentIndex = selectedRow
    }

    function scheduleSelectionAlignment() {
        if (selectionAlignmentPending) {
            return
        }
        selectionAlignmentPending = true
        Qt.callLater(() => {
            selectionAlignmentPending = false
            synchronizeCurrentIndexWithSelection()
        })
        selectionAlignmentTimer.restart()
    }

    function synchronizeCurrentIndexWithSelection() {
        if (restoringIndex) {
            return
        }
        alignCurrentIndexWithSelection()
        rememberCurrentPath()
        updateSelectedMediaPath()
    }

    function confirmTrashSelected() {
        if (!browserModel || browserModel.selectedPaths.length === 0) {
            return
        }
        root.trashConfirmationRequested(browserModel.selectedPaths.length)
    }

    function confirmDeleteSelectedPermanently() {
        if (!browserModel || browserModel.selectedPaths.length === 0) {
            return
        }
        root.deleteConfirmationRequested(browserModel.selectedPaths.length)
    }

    function trashSelected() {
        if (!browserModel) {
            return { "ok": false, "error": "No model" }
        }
        const rows = selectedRows()
        if (rows.length > 0) {
            let maxRow = -1
            for (let i = 0; i < rows.length; i += 1) {
                const row = rows[i]
                if (row > maxRow) {
                    maxRow = row
                }
            }
            pendingDeletionIndex = maxRow
            pendingDeletionFocus = true
            lastSelectedPath = ""
        } else {
            pendingDeletionIndex = -1
            pendingDeletionFocus = false
        }
        if (browserModel.startMoveSelectedToTrash) {
            browserModel.startMoveSelectedToTrash()
            return { "ok": true, "pending": pendingTrue }
        }
        if (browserModel.moveSelectedToTrash) {
            const result = browserModel.moveSelectedToTrash()
            if (!result || !result.ok) {
                pendingDeletionIndex = -1
                pendingDeletionFocus = false
            }
            return result
        }
        return { "ok": false, "error": "Trash not supported" }
    }

    function deleteSelectedPermanently() {
        if (!browserModel) {
            return { "ok": false, "error": "No model" }
        }
        const rows = selectedRows()
        if (rows.length > 0) {
            let maxRow = -1
            for (let i = 0; i < rows.length; i += 1) {
                const row = rows[i]
                if (row > maxRow) {
                    maxRow = row
                }
            }
            pendingDeletionIndex = maxRow
            pendingDeletionFocus = true
            lastSelectedPath = ""
        } else {
            pendingDeletionIndex = -1
            pendingDeletionFocus = false
        }
        if (browserModel.startDeleteSelectedPermanently) {
            browserModel.startDeleteSelectedPermanently()
            return { "ok": true, "pending": pendingTrue }
        }
        if (browserModel.deleteSelectedPermanently) {
            const result = browserModel.deleteSelectedPermanently()
            if (!result || !result.ok) {
                pendingDeletionIndex = -1
                pendingDeletionFocus = false
            }
            return result
        }
        return { "ok": false, "error": "Delete not supported" }
    }

    function requestRenameSelected() {
        if (!browserModel) {
            return
        }
        const paths = browserModel.selectedPaths || []
        if (paths.length !== 1) {
            return
        }
        requestRenamePath(paths[0])
    }

    function requestRenamePath(path) {
        if (!browserModel || !path) {
            return
        }
        renameDialog.sourcePath = path
        renameDialog.currentText = baseNameFromPath(path)
        renameDialog.description = qsTr("Current: %1").arg(baseNameFromPath(path))
        renameDialog.open()
    }

    function requestCreateFolder(path) {
        if (!browserModel || !path) {
            return
        }
        createFolderDialog.parentPath = path
        createFolderDialog.currentText = ""
        createFolderDialog.description = qsTr("Parent: %1").arg(path)
        createFolderDialog.open()
    }

    Shortcut {
        sequences: ["F5"]
        context: Qt.ApplicationShortcut
        enabled: root.hasFocus && !root.textInputActive
        onActivated: {
            if (browserModel) {
                browserModel.refresh()
            }
        }
    }

    Shortcut {
        sequences: ["F2"]
        context: Qt.ApplicationShortcut
        enabled: root.hasFocus && !root.textInputActive
        onActivated: root.requestRenameSelected()
    }

    Pane {
        anchors.fill: parent
        padding: Theme.panelPadding
        Material.background: panelBackground

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spaceMd

            FolderViewToolBar {
                id: toolbar
                Layout.fillWidth: true
                browserModel: root.browserModel
                volumeModel: volumeModel
                volumeUpdating: root.volumeUpdating
                previousFocusItem: root.previousPanelFocusItem
                nextFocusItem: sortBar.refreshButton
                onGoUpRequested: {
                    root.goUp()
                }
            }

            FolderViewSortBar {
                id: sortBar
                Layout.fillWidth: true
                browserModel: root.browserModel
                previousFocusItem: toolbar.upButton
                nextFocusItem: browserGrid.focusItem
                onSortKeyChangedByUser: (sortKey) => root.sortKeyChangedByUser(sortKey)
                onSortOrderChangedByUser: (sortOrder) => root.sortOrderChangedByUser(sortOrder)
                onDirsFirstChangedByUser: (enabled) => root.dirsFirstChangedByUser(enabled)
                onNameFilterChangedByUser: (filter) => root.nameFilterChangedByUser(filter)
                onMinimumByteSizeChangedByUser: (value) => root.minimumByteSizeChangedByUser(value)
                onMaximumByteSizeChangedByUser: (value) => root.maximumByteSizeChangedByUser(value)
                onMinimumImageWidthChangedByUser: (value) => root.minimumImageWidthChangedByUser(value)
                onMaximumImageWidthChangedByUser: (value) => root.maximumImageWidthChangedByUser(value)
                onMinimumImageHeightChangedByUser: (value) => root.minimumImageHeightChangedByUser(value)
                onMaximumImageHeightChangedByUser: (value) => root.maximumImageHeightChangedByUser(value)
            }

            FolderViewGrid {
                id: browserGrid
                Layout.fillWidth: true
                Layout.fillHeight: true
                browserModel: root.browserModel
                syncEnabled: root.syncEnabled
                statusPending: root.statusPending
                statusIdentical: root.statusIdentical
                statusDifferent: root.statusDifferent
                statusIdenticalColor: root.statusIdenticalColor
                statusDifferentColor: root.statusDifferentColor
                tabFocusItem: root.nextPanelFocusItem
                backtabFocusItem: sortBar.clearButton
                onViewSyncRequested: root.viewSyncRequested()
                onCopyLeftRequested: root.copyLeftRequested()
                onCopyRightRequested: root.copyRightRequested()
                onMoveLeftRequested: root.moveLeftRequested()
                onMoveRightRequested: root.moveRightRequested()
                onMoveOtherRequested: root.moveOtherRequested()
                onCopyOtherRequested: root.copyOtherRequested()
                onBackgroundClicked: {
                    root.forceActiveFocus()
                    if (browserGrid.focusItem) {
                        browserGrid.focusItem.forceActiveFocus()
                    }
                }
                onRenameRequested: (path) => root.requestRenamePath(path)
                onCreateFolderRequested: (path) => root.requestCreateFolder(path)
                onTrashRequested: root.confirmTrashSelected()
                onDeleteRequested: root.confirmDeleteSelectedPermanently()
                onBackupOperationFinished: (message) => root.renameSucceeded(message)
                onBackupOperationError: (message) => root.errorRaised(message)
                onCurrentIndexUpdated: (value) => {
                    if (root.restoringIndex) {
                        return
                    }
                    root.lastCurrentIndex = value
                    root.synchronizeCurrentIndexWithSelection()
                    root.rememberCurrentPath()
                }
            }
        }
    }

    InputDialog {
        id: renameDialog
        parent: root.Window.window ? root.Window.window.contentItem : null
        property string sourcePath: ""
        titleText: qsTr("Rename")
        placeholderText: qsTr("New name")
        selectAllOnOpen: true
        onTextAccepted: (newName) => {
            const path = renameDialog.sourcePath
            if (!browserModel || !browserModel.renamePath) {
                const fallbackError = qsTr("Rename failed")
                root.errorRaised(fallbackError)
                return
            }
            const result = browserModel.renamePath(path, newName)
            if (!result || !result.ok) {
                const renameError = result && result.error ? result.error : qsTr("Rename failed")
                root.errorRaised(renameError)
                return
            }
            const targetName = result.newPath ? baseNameFromPath(result.newPath) : newName
            renameSucceeded(qsTr("Renamed to %1").arg(targetName))
        }
    }

    InputDialog {
        id: createFolderDialog
        parent: root.Window.window ? root.Window.window.contentItem : null
        property string parentPath: ""
        titleText: qsTr("Create folder")
        placeholderText: qsTr("Folder name")
        onTextAccepted: (folderName) => {
            const path = createFolderDialog.parentPath
            if (!browserModel || !browserModel.createFolder) {
                const fallbackError = qsTr("Create folder failed")
                root.errorRaised(fallbackError)
                return
            }
            const created = browserModel.createFolder(path, folderName)
            if (created) {
                browserModel.refresh()
                root.renameSucceeded(qsTr("Folder created"))
            } else {
                root.errorRaised(qsTr("Failed to create folder"))
            }
        }
    }


    Connections {
        target: browserModel
        function onCopyFinished(result) {
            root.copyFinished(result)
        }
        function onTrashFinished(result) {
            root.trashFinished(result)
        }
        function onLoadingChanged() {
            if (!root.browserModel) {
                return
            }
            if (root.browserModel.loading) {
                root.rememberScrollOffset()
                return
            }
            if (root.pendingIndexReset) {
                root.pendingIndexReset = false
                if (browserGrid.count > 0) {
                    let nextIndex = -1
                    if (browserModel.selectedRows) {
                        const rows = browserModel.selectedRows()
                        if (rows && rows.length > 0) {
                            nextIndex = rows[0]
                        }
                    }
                    if (nextIndex < 0) {
                        nextIndex = 0
                    }
                    browserGrid.setCurrentIndex(nextIndex)
                    if (browserModel.select && nextIndex >= 0) {
                        browserModel.select(nextIndex, false)
                    }
                    browserGrid.positionViewAtIndex(nextIndex)
                } else {
                    browserGrid.setCurrentIndex(-1)
                }
            } else {
                root.restoreScrollOffset()
            }
            if (root.pendingDeletionFocus) {
                const nextIndex = root.clampIndex(root.pendingDeletionIndex, browserGrid.count)
                if (nextIndex >= 0) {
                    browserGrid.setCurrentIndex(nextIndex)
                    browserGrid.positionViewAtIndex(nextIndex)
                    root.lastCurrentIndex = nextIndex
                    root.rememberCurrentPath()
                }
                root.pendingDeletionIndex = -1
                root.pendingDeletionFocus = false
            }
        }
        function onRootPathChanged() {
            if (toolbar.pathField.text !== browserModel.rootPath) {
                toolbar.pathField.text = browserModel.rootPath
            }
            root.pendingIndexReset = true
            const volumeIndex = volumeModel.indexForPath(browserModel.rootPath)
            if (volumeIndex >= 0 && toolbar.volumeCombo.currentIndex !== volumeIndex) {
                volumeUpdating = true
                toolbar.volumeCombo.currentIndex = volumeIndex
                volumeUpdating = false
            }
        }
        function onSortKeyChanged() {
            if (sortBar.sortCombo.currentIndex !== browserModel.sortKey) {
                sortBar.sortCombo.currentIndex = browserModel.sortKey
            }
        }
        function onSortOrderChanged() {
            const expected = browserModel.sortOrder === Qt.DescendingOrder
            if (sortBar.sortOrderBox.checked !== expected) {
                sortBar.sortOrderBox.checked = expected
            }
        }
        function onShowDirsFirstChanged() {
            if (sortBar.dirsFirstBox.checked !== browserModel.showDirsFirst) {
                sortBar.dirsFirstBox.checked = browserModel.showDirsFirst
            }
        }
        function onNameFilterChanged() {
            if (sortBar.nameFilterField.text !== browserModel.nameFilter) {
                sortBar.nameFilterField.text = browserModel.nameFilter
            }
        }
        function onMinimumByteSizeChanged() {
            const value = browserModel.minimumByteSize
            const textValue = value < 0 ? "" : String(value)
            if (sortBar.minimumByteSizeField.text !== textValue) {
                sortBar.minimumByteSizeField.text = textValue
            }
        }
        function onMaximumByteSizeChanged() {
            const value = browserModel.maximumByteSize
            const textValue = value < 0 ? "" : String(value)
            if (sortBar.maximumByteSizeField.text !== textValue) {
                sortBar.maximumByteSizeField.text = textValue
            }
        }
        function onMinimumImageWidthChanged() {
            const value = browserModel.minimumImageWidth
            const textValue = value < 0 ? "" : String(value)
            if (sortBar.minimumImageWidthField.text !== textValue) {
                sortBar.minimumImageWidthField.text = textValue
            }
        }
        function onMaximumImageWidthChanged() {
            const value = browserModel.maximumImageWidth
            const textValue = value < 0 ? "" : String(value)
            if (sortBar.maximumImageWidthField.text !== textValue) {
                sortBar.maximumImageWidthField.text = textValue
            }
        }
        function onMinimumImageHeightChanged() {
            const value = browserModel.minimumImageHeight
            const textValue = value < 0 ? "" : String(value)
            if (sortBar.minimumImageHeightField.text !== textValue) {
                sortBar.minimumImageHeightField.text = textValue
            }
        }
        function onMaximumImageHeightChanged() {
            const value = browserModel.maximumImageHeight
            const textValue = value < 0 ? "" : String(value)
            if (sortBar.maximumImageHeightField.text !== textValue) {
                sortBar.maximumImageHeightField.text = textValue
            }
        }
        function onFolderActivated(path) {
            browserModel.rootPath = path
            root.folderActivated(path)
        }
        function onFileActivated(path) {
            root.fileActivated(path)
        }
        function onSelectedPathsChanged() {
            root.selectionChanged(browserModel.selectedPaths)
            if (!root.restoringIndex) {
                root.synchronizeCurrentIndexWithSelection()
                root.scheduleSelectionAlignment()
            }
        }
        function onSelectedIsImageChanged() {
            root.updateSelectedMediaPath()
        }
        function onSelectedIsVideoChanged() {
            root.updateSelectedMediaPath()
        }
        function onModelAboutToBeReset() {
            root.restoringIndex = true
            root.restoreCurrentIndex = root.pendingDeletionIndex >= 0
                ? root.pendingDeletionIndex
                : root.lastCurrentIndex
            root.restoreAdjacentPath = root.adjacentPathForRow(root.restoreCurrentIndex)
            if (root.restoreCurrentIndex < 0 && root.lastSelectedPath === "") {
                root.restoringIndex = false
                root.restoreAdjacentPath = ""
            }
        }
        function onModelReset() {
            root.applyRestore()
        }
    }

    Component.onCompleted: {
        if (!browserModel) {
            return
        }
        sortBar.sortCombo.currentIndex = browserModel.sortKey
        sortBar.sortOrderBox.checked = browserModel.sortOrder === Qt.DescendingOrder
        sortBar.dirsFirstBox.checked = browserModel.showDirsFirst
        sortBar.nameFilterField.text = browserModel.nameFilter
        sortBar.minimumByteSizeField.text = browserModel.minimumByteSize < 0 ? "" : String(browserModel.minimumByteSize)
        sortBar.maximumByteSizeField.text = browserModel.maximumByteSize < 0 ? "" : String(browserModel.maximumByteSize)
        sortBar.minimumImageWidthField.text = browserModel.minimumImageWidth < 0 ? "" : String(browserModel.minimumImageWidth)
        sortBar.maximumImageWidthField.text = browserModel.maximumImageWidth < 0 ? "" : String(browserModel.maximumImageWidth)
        sortBar.minimumImageHeightField.text = browserModel.minimumImageHeight < 0 ? "" : String(browserModel.minimumImageHeight)
        sortBar.maximumImageHeightField.text = browserModel.maximumImageHeight < 0 ? "" : String(browserModel.maximumImageHeight)
        const volumeIndex = volumeModel.indexForPath(browserModel.rootPath)
        if (volumeIndex >= 0) {
            volumeUpdating = true
            toolbar.volumeCombo.currentIndex = volumeIndex
            volumeUpdating = false
        }
        updateSelectedMediaPath()
    }

    function syncRootPath(sourceModel, targetModel) {
        if (rootPathSyncing || !sourceModel || !targetModel) {
            return
        }
        if (sourceModel.rootPath === targetModel.rootPath) {
            return
        }
        rootPathSyncing = true
        targetModel.rootPath = sourceModel.rootPath
        rootPathSyncing = false
    }

    Connections {
        target: defaultModel
        function onRootPathChanged() {
            root.syncRootPath(defaultModel, root.externalModel)
        }
    }

    Connections {
        target: root.externalModel
        function onRootPathChanged() {
            root.syncRootPath(root.externalModel, defaultModel)
        }
    }
}
