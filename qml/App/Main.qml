import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import "../Components"
import Galman 1.0

ApplicationWindow {
    id: window
    width: 1200
    height: 800
    visible: true
    title: qsTr("Galman")
    property color panelBackground: Theme.panelBackground
    property int copyBarSize: 40
    property var errorQueue: []
    property string currentError: ""
    property bool statusActive: false
    property int emptyItemCount: 0
    property int singleItemCount: 1
    property bool copyInProgress: leftBrowser.copyInProgress || rightBrowser.copyInProgress
    property real copyProgress: leftBrowser.copyInProgress
        ? leftBrowser.copyProgress
        : (rightBrowser.copyInProgress ? rightBrowser.copyProgress : 0)
    property bool singlePreviewEnabled: false
    property var singlePreviewBrowser: null
    property var comparePreviewBrowser: null
    property var activeSelectionPane: null
    Material.theme: Material.Dark
    Material.primary: Material.Blue
    Material.accent: Material.DeepOrange
    Overlay.modal: Rectangle {
        color: Theme.modalOverlayColor
    }

    FolderCompareModel {
        id: compareModel
        onEnabledChanged: {
            if (enabled) {
                window.singlePreviewEnabled = false
                window.singlePreviewBrowser = null
            }
        }
    }

    Binding {
        target: compareModel
        property: "hideIdentical"
        value: leftPanel.hideIdentical
    }

    property bool compareSyncing: false
    property bool navigationSyncing: false
    property var activeCopySourcePane: null
    property var activeCopyTargetPane: null
    property var activeCopySourcePaths: []
    property bool activeCopyCompare: false
    property string activeCopyOperation: "copy"
    property var activeTrashSourcePane: null
    property var activeTrashSourcePaths: []

    function syncBrowserState(source, target) {
        if (!compareModel.enabled || compareModel.loading || compareSyncing || !source || !target) {
            return
        }
        compareSyncing = true
        const rows = source.selectedRows()
        if (rows && rows.length > 0) {
            target.setSelectionRows(rows, false)
        } else {
            target.clearSelection()
        }
        target.setCurrentIndex(source.currentIndex())
        target.setScrollOffset(source.scrollOffset())
        compareSyncing = false
    }

    function syncBrowserSettings(source, target) {
        if (!compareModel.enabled || compareModel.loading || compareSyncing || !source || !target || !source.browserModel || !target.browserModel) {
            return
        }
        compareSyncing = true
        target.browserModel.sortKey = source.browserModel.sortKey
        target.browserModel.sortOrder = source.browserModel.sortOrder
        target.browserModel.showDirsFirst = source.browserModel.showDirsFirst
        target.browserModel.nameFilter = source.browserModel.nameFilter
        target.browserModel.minimumByteSize = source.browserModel.minimumByteSize
        target.browserModel.maximumByteSize = source.browserModel.maximumByteSize
        target.browserModel.minimumImageWidth = source.browserModel.minimumImageWidth
        target.browserModel.maximumImageWidth = source.browserModel.maximumImageWidth
        target.browserModel.minimumImageHeight = source.browserModel.minimumImageHeight
        target.browserModel.maximumImageHeight = source.browserModel.maximumImageHeight
        compareSyncing = false
    }

    function triggerCopyLeftToRight() {
        if (leftBrowser.selectionCount === 0) {
            return
        }
        confirmDialog.action = "copy"
        confirmDialog.sourcePane = leftBrowser
        confirmDialog.targetPane = rightBrowser
        confirmDialog.directionText = qsTr("left to right")
        const stats = leftBrowser.selectionStats()
        confirmDialog.fileCount = stats.files || 0
        confirmDialog.dirCount = stats.dirs || 0
        confirmDialog.itemCount = leftBrowser.selectionCount
        confirmDialog.nameConflictCount = leftBrowser.copyNameConflictCount(rightBrowser.currentPath)
        confirmDialog.open()
    }

    function triggerCopyRightToLeft() {
        if (rightBrowser.selectionCount === 0) {
            return
        }
        confirmDialog.action = "copy"
        confirmDialog.sourcePane = rightBrowser
        confirmDialog.targetPane = leftBrowser
        confirmDialog.directionText = qsTr("right to left")
        const stats = rightBrowser.selectionStats()
        confirmDialog.fileCount = stats.files || 0
        confirmDialog.dirCount = stats.dirs || 0
        confirmDialog.itemCount = rightBrowser.selectionCount
        confirmDialog.nameConflictCount = rightBrowser.copyNameConflictCount(leftBrowser.currentPath)
        confirmDialog.open()
    }

    function triggerMoveLeftToRight() {
        if (leftBrowser.selectionCount === 0) {
            return
        }
        confirmDialog.action = "move"
        confirmDialog.sourcePane = leftBrowser
        confirmDialog.targetPane = rightBrowser
        confirmDialog.directionText = qsTr("left to right")
        const stats = leftBrowser.selectionStats()
        confirmDialog.fileCount = stats.files || 0
        confirmDialog.dirCount = stats.dirs || 0
        confirmDialog.itemCount = leftBrowser.selectionCount
        confirmDialog.nameConflictCount = leftBrowser.copyNameConflictCount(rightBrowser.currentPath)
        confirmDialog.open()
    }

    function triggerMoveRightToLeft() {
        if (rightBrowser.selectionCount === 0) {
            return
        }
        confirmDialog.action = "move"
        confirmDialog.sourcePane = rightBrowser
        confirmDialog.targetPane = leftBrowser
        confirmDialog.directionText = qsTr("right to left")
        const stats = rightBrowser.selectionStats()
        confirmDialog.fileCount = stats.files || 0
        confirmDialog.dirCount = stats.dirs || 0
        confirmDialog.itemCount = rightBrowser.selectionCount
        confirmDialog.nameConflictCount = rightBrowser.copyNameConflictCount(leftBrowser.currentPath)
        confirmDialog.open()
    }

    function focusedBrowser() {
        if (leftBrowser.hasFocus) {
            return leftBrowser
        }
        if (rightBrowser.hasFocus) {
            return rightBrowser
        }
        return leftBrowser
    }

    function focusOtherBrowser() {
        const current = focusedBrowser()
        const target = current === leftBrowser ? rightBrowser : leftBrowser
        if (!target) {
            return
        }
        if (target.lastFocusItem) {
            target.lastFocusItem.forceActiveFocus()
            return
        }
        target.forceActiveFocus()
    }

    function openInternalPreviewForBrowser(browser) {
        if (!browser) {
            return
        }
        if (compareModel.enabled) {
            comparePreviewBrowser = browser
            leftPanel.syncPreviewEnabled = true
            Qt.callLater(() => {
                if (comparePreview && comparePreview.visible) {
                    comparePreview.forceActiveFocus()
                }
            })
            return
        }
        singlePreviewBrowser = browser
        singlePreviewEnabled = true
    }

    function openExternalFileForBrowser(browser) {
        if (!browser || !browser.browserModel) {
            return
        }
        const row = browser.lastCurrentIndex
        if (row < 0) {
            return
        }
        if (browser.browserModel.isDir && browser.browserModel.isDir(row)) {
            return
        }
        if (browser.browserModel.pathForRow) {
            const path = browser.browserModel.pathForRow(row)
            if (path) {
                Qt.openUrlExternally("file://" + path)
            }
        }
    }

    function startCopy(sourcePane, targetPane) {
        if (!sourcePane || !targetPane) {
            return
        }
        activeCopySourcePane = sourcePane
        activeCopyTargetPane = targetPane
        activeCopySourcePaths = sourcePane.selectedPaths()
        activeCopyCompare = compareModel.enabled
        activeCopyOperation = "copy"
        sourcePane.startCopySelectionTo(targetPane.currentPath)
    }

    function startMove(sourcePane, targetPane) {
        if (!sourcePane || !targetPane) {
            return
        }
        activeCopySourcePane = sourcePane
        activeCopyTargetPane = targetPane
        activeCopySourcePaths = sourcePane.selectedPaths()
        activeCopyCompare = compareModel.enabled
        activeCopyOperation = "move"
        sourcePane.startMoveSelectionTo(targetPane.currentPath)
    }

    function stopCopy() {
        if (leftBrowser.copyInProgress) {
            leftBrowser.cancelCopy()
        } else if (rightBrowser.copyInProgress) {
            rightBrowser.cancelCopy()
        }
    }

    function startRemovalSelection(sourcePane, moveToTrash) {
        if (!sourcePane) {
            return
        }
        activeTrashSourcePane = sourcePane
        activeTrashSourcePaths = sourcePane.selectedPaths()
        const result = moveToTrash ? sourcePane.trashSelected() : sourcePane.deleteSelectedPermanently()
        if (result && result.error) {
            pushError(result.error)
            activeTrashSourcePane = null
            activeTrashSourcePaths = []
        }
    }

    function startTrashSelection(sourcePane) {
        startRemovalSelection(sourcePane, true)
    }

    function startDeleteSelection(sourcePane) {
        startRemovalSelection(sourcePane, false)
    }

    function refreshDisplays() {
        if (comparePreview && comparePreview.refreshImages) {
            comparePreview.refreshImages()
        }
        if (singlePreview && singlePreview.refreshImages) {
            singlePreview.refreshImages()
        }
    }

    function handleCopyFinished(sourcePane, result) {
        if (sourcePane !== activeCopySourcePane) {
            return
        }
        if (result && result.error) {
            pushError(result.error)
        }
        const operationKey = activeCopyOperation === "move" ? "moved" : "copied"
        const transferred = result && result[operationKey] ? result[operationKey] : 0
        const targetPaths = transferred > emptyItemCount && activeCopyTargetPane
            ? buildTargetPaths(activeCopySourcePaths, activeCopyTargetPane.currentPath)
            : []
        if (transferred > emptyItemCount && !result.cancelled) {
            const hasSingleItem = activeCopySourcePaths
                && activeCopySourcePaths.length === singleItemCount
            if (hasSingleItem && activeCopyTargetPane) {
                const destinationPath = targetPaths.length > emptyItemCount ? targetPaths[emptyItemCount] : ""
                if (destinationPath !== "") {
                    const statusText = activeCopyOperation === "move"
                        ? qsTr("Move completed: %1")
                        : qsTr("Copy completed: %1")
                    pushStatus(statusText.arg(destinationPath))
                } else {
                    pushStatus(activeCopyOperation === "move"
                        ? qsTr("Move completed")
                        : qsTr("Copy completed"))
                }
            } else {
                pushStatus(activeCopyOperation === "move"
                    ? qsTr("Move completed")
                    : qsTr("Copy completed"))
            }
        }
        if (transferred > emptyItemCount && activeCopyTargetPane) {
            if (activeCopyCompare) {
                if (activeCopyOperation === "move") {
                    if (sourcePane === leftBrowser) {
                        compareModel.refreshFiles(activeCopySourcePaths, targetPaths)
                    } else {
                        compareModel.refreshFiles(targetPaths, activeCopySourcePaths)
                    }
                } else if (sourcePane === leftBrowser) {
                    compareModel.refreshFiles([], targetPaths)
                } else {
                    compareModel.refreshFiles(targetPaths, [])
                }
            } else {
                if (activeCopyOperation === "move" && activeCopySourcePane) {
                    activeCopySourcePane.refreshPaths(activeCopySourcePaths)
                }
                activeCopyTargetPane.refreshPaths(targetPaths)
            }
        }
        if (transferred > emptyItemCount && !result.cancelled) {
            refreshDisplays()
        }
        activeCopySourcePane = null
        activeCopyTargetPane = null
        activeCopySourcePaths = []
        activeCopyCompare = false
        activeCopyOperation = "copy"
    }

    function handleTrashFinished(sourcePane, result) {
        if (sourcePane !== activeTrashSourcePane) {
            return
        }
        if (result && result.error) {
            pushError(result.error)
        }
        const moved = result && result.moved ? result.moved : emptyItemCount
        if (moved > emptyItemCount && !result.cancelled) {
            const hasSingleItem = activeTrashSourcePaths
                && activeTrashSourcePaths.length === singleItemCount
            if (hasSingleItem) {
                const deletedName = baseNameFromPath(activeTrashSourcePaths[emptyItemCount])
                pushStatus(qsTr("Deleted: %1").arg(deletedName))
            } else {
                pushStatus(qsTr("Delete completed"))
            }
            refreshDisplays()
        }
        activeTrashSourcePane = null
        activeTrashSourcePaths = []
    }

    function buildTargetPaths(sourcePaths, targetDir) {
        const targets = []
        if (!sourcePaths || !targetDir) {
            return targets
        }
        for (let i = 0; i < sourcePaths.length; i += 1) {
            const parts = sourcePaths[i].split("/")
            const name = parts.length > 0 ? parts[parts.length - 1] : ""
            if (name !== "") {
                targets.push(targetDir + "/" + name)
            }
        }
        return targets
    }

    function pushError(message) {
        if (!message || String(message).trim() === "") {
            return
        }
        const text = String(message)
        if (statusActive) {
            statusTimer.stop()
            statusActive = false
        }
        if (currentError === "") {
            currentError = text
            return
        }
        errorQueue.push(text)
        if (errorQueue.length === 1) {
            errorTimer.restart()
        }
    }

    function pushStatus(message) {
        if (!message || String(message).trim() === "") {
            return
        }
        currentError = String(message)
        statusActive = true
        statusTimer.restart()
    }

    function advanceErrorQueue() {
        if (errorQueue.length === 0) {
            return
        }
        currentError = errorQueue.shift()
        if (errorQueue.length > 0) {
            errorTimer.restart()
        }
    }

    Timer {
        id: errorTimer
        interval: 5000
        repeat: false
        onTriggered: advanceErrorQueue()
    }

    Timer {
        id: statusTimer
        interval: 2500
        repeat: false
        onTriggered: {
            statusActive = false
        }
    }

    function goUpFocusedPane() {
        if (leftBrowser.hasFocus) {
            leftBrowser.browserModel.goUp()
            if (compareModel.enabled) {
                rightBrowser.browserModel.goUp()
            }
            return
        }
        if (rightBrowser.hasFocus) {
            rightBrowser.browserModel.goUp()
            if (compareModel.enabled) {
                leftBrowser.browserModel.goUp()
            }
        }
    }

    function baseNameFromPath(path) {
        const normalized = String(path || "").replace(/\\/g, "/")
        const idx = normalized.lastIndexOf("/")
        return idx >= 0 ? normalized.slice(idx + 1) : normalized
    }

    function favoritePairLabel(leftPath, rightPath) {
        const leftName = baseNameFromPath(leftPath)
        const rightName = baseNameFromPath(rightPath)
        const leftText = leftName !== "" ? leftName : String(leftPath || "")
        const rightText = rightName !== "" ? rightName : String(rightPath || "")
        return leftText + " <-> " + rightText
    }

    function applyFavoritePair(leftPath, rightPath) {
        if (!leftPath || !rightPath) {
            pushError(qsTr("Cannot apply favorite pair with empty folder"))
            return
        }
        leftBrowser.browserModel.rootPath = leftPath
        rightBrowser.browserModel.rootPath = rightPath
    }

    function saveCurrentFavoritePair() {
        if (!favoritesManager) {
            return
        }
        const leftPath = leftBrowser.currentPath
        const rightPath = rightBrowser.currentPath
        if (leftPath === "" || rightPath === "") {
            pushError(qsTr("Cannot save empty favorite pair"))
            return
        }
        const added = favoritesManager.addFavoritePair(leftPath, rightPath)
        if (added) {
            pushStatus(qsTr("Favorite pair saved"))
        } else {
            pushStatus(qsTr("Favorite pair already saved"))
        }
    }

    function syncEnterFolder(sourcePane, targetPane, path) {
        if (!compareModel.enabled || navigationSyncing || !sourcePane || !targetPane) {
            return
        }
        const name = baseNameFromPath(path)
        if (name === "") {
            return
        }
        const targetPath = targetPane.findDirectoryPathByName(name)
        if (targetPath === "") {
            return
        }
        navigationSyncing = true
        targetPane.browserModel.rootPath = targetPath
        navigationSyncing = false
    }

    function syncGoUp(sourcePane, targetPane) {
        if (!compareModel.enabled || navigationSyncing || !sourcePane || !targetPane) {
            return
        }
        navigationSyncing = true
        targetPane.browserModel.goUp()
        navigationSyncing = false
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem {
                text: qsTr("Quit")
                onTriggered: Qt.quit()
            }
        }

        Menu {
            id: favoritesMenu
            title: qsTr("Favorites")

            Instantiator {
                model: (!favoritesManager || favoritesManager.favoritePairs.length === 0) ? [true] : []
                delegate: MenuItem {
                    text: qsTr("No favorites saved")
                    enabled: false
                }
                onObjectAdded: (index, object) => favoritesMenu.addItem(object)
                onObjectRemoved: (index, object) => favoritesMenu.removeItem(object)
            }

            Instantiator {
                model: favoritesManager ? favoritesManager.favoritePairs : []
                delegate: MenuItem {
                    text: favoritePairLabel(modelData.leftPath, modelData.rightPath)
                    onTriggered: {
                        applyFavoritePair(modelData.leftPath, modelData.rightPath)
                    }
                }
                onObjectAdded: (index, object) => favoritesMenu.addItem(object)
                onObjectRemoved: (index, object) => favoritesMenu.removeItem(object)
            }
        }

        Menu {
            id: scriptsMenu
            title: qsTr("Scripts")

            Instantiator {
                model: (!scriptManager || scriptManager.scripts.length === 0) ? [true] : []
                delegate: MenuItem {
                    text: qsTr("No scripts found")
                    enabled: false
                }
                onObjectAdded: (index, object) => scriptsMenu.addItem(object)
                onObjectRemoved: (index, object) => scriptsMenu.removeItem(object)
            }

            Instantiator {
                model: scriptManager ? scriptManager.scripts : []
                delegate: MenuItem {
                    text: modelData.name
                    onTriggered: {
                        const meta = scriptEngine.loadScript(modelData.path)
                        if (meta.ok) {
                            leftPanel.scriptPath = modelData.path
                            leftPanel.scriptControls = meta.controls || []
                            leftPanel.scriptName = meta.name || modelData.name
                            leftPanel.scriptDescription = meta.description || ""
                        }
                    }
                }
                onObjectAdded: (index, object) => scriptsMenu.addItem(object)
                onObjectRemoved: (index, object) => scriptsMenu.removeItem(object)
            }
        }

        Menu {
            id: languageMenu
            title: qsTr("Language")
            Instantiator {
                model: languageManager ? languageManager.availableLanguages : []
                delegate: MenuItem {
                    text: modelData.name
                    checkable: true
                    checked: languageManager && languageManager.currentLanguage === modelData.code
                    onTriggered: {
                        if (languageManager) {
                            languageManager.setLanguage(modelData.code)
                        }
                    }
                }
                onObjectAdded: (index, object) => languageMenu.addItem(object)
                onObjectRemoved: (index, object) => languageMenu.removeItem(object)
            }
        }

        Menu {
            title: qsTr("Help")
            MenuItem {
                text: qsTr("Shortcuts")
                onTriggered: shortcutsDialog.open()
            }
        }
    }

    Shortcut {
        sequences: ["Backspace"]
        context: Qt.ApplicationShortcut
        enabled: (leftBrowser.hasFocus || rightBrowser.hasFocus)
            && !(leftBrowser.textInputActive || rightBrowser.textInputActive)
        onActivated: {
            goUpFocusedPane()
        }
    }

    Shortcut {
        sequences: ["Ctrl+S"]
        context: Qt.ApplicationShortcut
        enabled: !(leftBrowser.textInputActive || rightBrowser.textInputActive)
            && !confirmDialog.visible
        onActivated: {
            leftPanel.syncEnabled = !leftPanel.syncEnabled
        }
    }

    Shortcut {
        sequences: ["Ctrl+F"]
        context: Qt.ApplicationShortcut
        enabled: !(leftBrowser.textInputActive || rightBrowser.textInputActive)
            && !confirmDialog.visible
        onActivated: {
            saveCurrentFavoritePair()
        }
    }

    Shortcut {
        sequences: ["Ctrl+H"]
        context: Qt.ApplicationShortcut
        enabled: !(leftBrowser.textInputActive || rightBrowser.textInputActive)
            && !confirmDialog.visible
        onActivated: {
            leftPanel.hideIdentical = !leftPanel.hideIdentical
        }
    }

    Shortcut {
        sequences: ["Delete"]
        context: Qt.ApplicationShortcut
        enabled: (leftBrowser.hasFocus || rightBrowser.hasFocus)
            && !(leftBrowser.textInputActive || rightBrowser.textInputActive)
            && !confirmDialog.visible
        onActivated: {
            if (leftBrowser.hasFocus) {
                leftBrowser.confirmTrashSelected()
                return
            }
            if (rightBrowser.hasFocus) {
                rightBrowser.confirmTrashSelected()
            }
        }
    }

    Shortcut {
        sequences: ["Shift+Delete"]
        context: Qt.ApplicationShortcut
        enabled: (leftBrowser.hasFocus || rightBrowser.hasFocus)
            && !(leftBrowser.textInputActive || rightBrowser.textInputActive)
            && !confirmDialog.visible
        onActivated: {
            if (leftBrowser.hasFocus) {
                leftBrowser.confirmDeleteSelectedPermanently()
                return
            }
            if (rightBrowser.hasFocus) {
                rightBrowser.confirmDeleteSelectedPermanently()
            }
        }
    }

    Shortcut {
        sequences: ["Ctrl+Return", "Ctrl+Enter"]
        context: Qt.ApplicationShortcut
        enabled: !(leftBrowser.textInputActive || rightBrowser.textInputActive)
            && !confirmDialog.visible
        onActivated: {
            openExternalFileForBrowser(focusedBrowser())
        }
    }

    Shortcut {
        sequences: ["Alt+Left", "Alt+Right", "Alt+Up", "Alt+Down"]
        context: Qt.ApplicationShortcut
        enabled: !(leftBrowser.textInputActive || rightBrowser.textInputActive)
            && !confirmDialog.visible
        onActivated: focusOtherBrowser()
    }

    Shortcut {
        sequences: ["Left"]
        context: Qt.ApplicationShortcut
        enabled: compareModel.enabled
            && leftPanel.syncPreviewEnabled
            && !confirmDialog.visible
        onActivated: {
            const target = comparePreviewBrowser ? comparePreviewBrowser : leftBrowser
            if (target) {
                target.selectAdjacentImage(-1)
            }
        }
    }

    Shortcut {
        sequences: ["Right"]
        context: Qt.ApplicationShortcut
        enabled: compareModel.enabled
            && leftPanel.syncPreviewEnabled
            && !confirmDialog.visible
        onActivated: {
            const target = comparePreviewBrowser ? comparePreviewBrowser : leftBrowser
            if (target) {
                target.selectAdjacentImage(1)
            }
        }
    }

    Shortcut {
        sequences: ["Home"]
        context: Qt.ApplicationShortcut
        enabled: compareModel.enabled
            && leftPanel.syncPreviewEnabled
            && !confirmDialog.visible
        onActivated: {
            const target = comparePreviewBrowser ? comparePreviewBrowser : leftBrowser
            if (target) {
                target.selectBoundaryImage(true)
            }
        }
    }

    Shortcut {
        sequences: ["End"]
        context: Qt.ApplicationShortcut
        enabled: compareModel.enabled
            && leftPanel.syncPreviewEnabled
            && !confirmDialog.visible
        onActivated: {
            const target = comparePreviewBrowser ? comparePreviewBrowser : leftBrowser
            if (target) {
                target.selectBoundaryImage(false)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.windowMargin
        spacing: Theme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spaceLg

            CommandPanel {
                id: leftPanel
                Layout.preferredWidth: (parent ? parent.width : window.width) * 0.15
                Layout.fillHeight: true
                panelBackground: window.panelBackground
                syncEnabled: compareModel.enabled
                previousPanelFocusItem: null
                nextPanelFocusItem: leftBrowser.firstFocusItem
                onErrorRaised: (message) => pushError(message)
                onSyncEnabledChanged: {
                    if (compareModel.enabled === syncEnabled) {
                        return
                    }
                    if (syncEnabled) {
                        compareModel.leftModel.rootPath = leftBrowser.currentPath
                        compareModel.rightModel.rootPath = rightBrowser.currentPath
                        compareModel.enabled = true
                        syncBrowserSettings(leftBrowser, rightBrowser)
                        syncBrowserState(leftBrowser, rightBrowser)
                    } else {
                        compareModel.enabled = false
                    }
                }
                onSyncPreviewEnabledChanged: {
                    if (syncPreviewEnabled) {
                        if (!comparePreviewBrowser) {
                            comparePreviewBrowser = window.activeSelectionPane
                                ? window.activeSelectionPane
                                : leftBrowser
                        }
                        Qt.callLater(() => {
                            if (comparePreview && comparePreview.visible) {
                                comparePreview.forceActiveFocus()
                            }
                        })
                    } else {
                        comparePreviewBrowser = null
                    }
                }
            }

            Item {
                id: viewHost
                Layout.fillWidth: true
                Layout.fillHeight: true

            GridLayout {
                id: viewContainer
                anchors.fill: parent
                rowSpacing: Theme.spaceLg
                columnSpacing: Theme.spaceLg
                visible: !(compareModel.enabled && leftPanel.syncPreviewEnabled) && !singlePreviewEnabled
                property bool verticalSplit: width > 0 && height > 0 && (width / height) < 1.2
                columns: verticalSplit ? 1 : 3
                rows: verticalSplit ? 3 : 1

            FolderView {
                id: leftBrowser
                useExternalModel: compareModel.enabled
                externalModel: compareModel.leftModel
                syncEnabled: compareModel.enabled && !compareModel.loading
                previousPanelFocusItem: leftPanel.lastFocusItem
                nextPanelFocusItem: copyPanel.firstFocusItem
                Layout.preferredWidth: viewContainer.verticalSplit ? -1 : 0
                Layout.minimumWidth: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.row: viewContainer.verticalSplit ? 0 : 0
                Layout.column: viewContainer.verticalSplit ? 0 : 0
                settingsKey: "browser/leftPath"
                panelBackground: window.panelBackground
                focus: true
                onFolderActivated: (path) => {
                    syncEnterFolder(leftBrowser, rightBrowser, path)
                }
                onFileActivated: (path) => {
                    openInternalPreviewForBrowser(leftBrowser)
                }
                onSelectionChanged: (paths) => {
                    if (leftBrowser.hasFocus) {
                        window.activeSelectionPane = leftBrowser
                    }
                    leftPanel.selectedPaths = paths
                    leftPanel.selectedIsImage = leftBrowser.selectedIsImage
                    leftPanel.selectedFileCount = leftBrowser.selectedFileCount
                    leftPanel.selectedTotalBytes = leftBrowser.selectedTotalBytes
                    scriptEngine.setSelection(paths)
                    syncBrowserState(leftBrowser, rightBrowser)
                }
                onTrashConfirmationRequested: (count) => {
                    confirmDialog.action = "trash"
                    confirmDialog.sourcePane = leftBrowser
                    confirmDialog.targetPane = null
                    confirmDialog.itemCount = count
                    confirmDialog.directionText = ""
                    confirmDialog.open()
                }
                onDeleteConfirmationRequested: (count) => {
                    confirmDialog.action = "delete"
                    confirmDialog.sourcePane = leftBrowser
                    confirmDialog.targetPane = null
                    confirmDialog.itemCount = count
                    confirmDialog.directionText = ""
                    confirmDialog.open()
                }
                onCopyLeftRequested: triggerCopyRightToLeft()
                onCopyRightRequested: triggerCopyLeftToRight()
                onCopyOtherRequested: triggerCopyLeftToRight()
                onMoveLeftRequested: triggerMoveRightToLeft()
                onMoveRightRequested: triggerMoveLeftToRight()
                onMoveOtherRequested: triggerMoveLeftToRight()
                onViewSyncRequested: syncBrowserState(leftBrowser, rightBrowser)
                onSortKeyChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onSortOrderChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onDirsFirstChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onNameFilterChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onMinimumByteSizeChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onMaximumByteSizeChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onMinimumImageWidthChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onMaximumImageWidthChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onMinimumImageHeightChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onMaximumImageHeightChangedByUser: syncBrowserSettings(leftBrowser, rightBrowser)
                onGoUpRequested: syncGoUp(leftBrowser, rightBrowser)
                onRenameSucceeded: (message) => pushStatus(message)
            }

            Item {
                id: copyPanel
                Layout.row: viewContainer.verticalSplit ? 1 : 0
                Layout.column: viewContainer.verticalSplit ? 0 : 1
                Layout.preferredWidth: viewContainer.verticalSplit ? -1 : window.copyBarSize
                Layout.minimumWidth: viewContainer.verticalSplit ? 0 : window.copyBarSize
                Layout.maximumWidth: viewContainer.verticalSplit ? -1 : window.copyBarSize
                Layout.preferredHeight: viewContainer.verticalSplit ? window.copyBarSize : -1
                Layout.minimumHeight: viewContainer.verticalSplit ? window.copyBarSize : 0
                Layout.fillWidth: viewContainer.verticalSplit
                Layout.fillHeight: !viewContainer.verticalSplit
                property Item previousPanelFocusItem: leftBrowser.lastFocusItem
                property Item nextPanelFocusItem: rightBrowser.firstFocusItem
                readonly property Item copyLeftFocusButton: viewContainer.verticalSplit
                    ? copyLeftButtonVertical
                    : copyLeftButtonHorizontal
                readonly property Item copyRightFocusButton: viewContainer.verticalSplit
                    ? copyRightButtonVertical
                    : copyRightButtonHorizontal
                readonly property Item firstFocusItem: copyLeftFocusButton
                readonly property Item lastFocusItem: copyRightFocusButton

                RowLayout {
                    anchors.centerIn: parent
                    spacing: Theme.spaceMd
                    visible: viewContainer.verticalSplit

                    ToolButton {
                        id: copyLeftButtonVertical
                        text: "\u2190"
                        font.pixelSize: 18
                        KeyNavigation.backtab: copyPanel.previousPanelFocusItem
                        KeyNavigation.tab: copyPanel.copyRightFocusButton
                        onClicked: {
                            triggerCopyRightToLeft()
                        }
                    }

                    ToolButton {
                        id: copyRightButtonVertical
                        text: "\u2192"
                        font.pixelSize: 18
                        KeyNavigation.backtab: copyPanel.copyLeftFocusButton
                        KeyNavigation.tab: copyPanel.nextPanelFocusItem
                        onClicked: {
                            triggerCopyLeftToRight()
                        }
                    }
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Theme.spaceMd
                    visible: !viewContainer.verticalSplit

                    ToolButton {
                        id: copyLeftButtonHorizontal
                        text: "\u2190"
                        font.pixelSize: 18
                        KeyNavigation.backtab: copyPanel.previousPanelFocusItem
                        KeyNavigation.tab: copyPanel.copyRightFocusButton
                        onClicked: {
                            triggerCopyRightToLeft()
                        }
                    }

                    ToolButton {
                        id: copyRightButtonHorizontal
                        text: "\u2192"
                        font.pixelSize: 18
                        KeyNavigation.backtab: copyPanel.copyLeftFocusButton
                        KeyNavigation.tab: copyPanel.nextPanelFocusItem
                        onClicked: {
                            triggerCopyLeftToRight()
                        }
                    }
                }

            }

            FolderView {
                id: rightBrowser
                useExternalModel: compareModel.enabled
                externalModel: compareModel.rightModel
                syncEnabled: compareModel.enabled && !compareModel.loading
                previousPanelFocusItem: copyPanel.lastFocusItem
                nextPanelFocusItem: leftPanel.firstFocusItem
                Layout.preferredWidth: viewContainer.verticalSplit ? -1 : 0
                Layout.minimumWidth: 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.row: viewContainer.verticalSplit ? 2 : 0
                Layout.column: viewContainer.verticalSplit ? 0 : 2
                settingsKey: "browser/rightPath"
                panelBackground: window.panelBackground
                focus: true
                onFolderActivated: (path) => {
                    syncEnterFolder(rightBrowser, leftBrowser, path)
                }
                onFileActivated: (path) => {
                    openInternalPreviewForBrowser(rightBrowser)
                }
                onSelectionChanged: (paths) => {
                    if (rightBrowser.hasFocus) {
                        window.activeSelectionPane = rightBrowser
                    }
                    leftPanel.selectedPaths = paths
                    leftPanel.selectedIsImage = rightBrowser.selectedIsImage
                    leftPanel.selectedFileCount = rightBrowser.selectedFileCount
                    leftPanel.selectedTotalBytes = rightBrowser.selectedTotalBytes
                    scriptEngine.setSelection(paths)
                    syncBrowserState(rightBrowser, leftBrowser)
                }
                onTrashConfirmationRequested: (count) => {
                    confirmDialog.action = "trash"
                    confirmDialog.sourcePane = rightBrowser
                    confirmDialog.targetPane = null
                    confirmDialog.itemCount = count
                    confirmDialog.directionText = ""
                    confirmDialog.open()
                }
                onDeleteConfirmationRequested: (count) => {
                    confirmDialog.action = "delete"
                    confirmDialog.sourcePane = rightBrowser
                    confirmDialog.targetPane = null
                    confirmDialog.itemCount = count
                    confirmDialog.directionText = ""
                    confirmDialog.open()
                }
                onCopyLeftRequested: triggerCopyRightToLeft()
                onCopyRightRequested: triggerCopyLeftToRight()
                onCopyOtherRequested: triggerCopyRightToLeft()
                onMoveLeftRequested: triggerMoveRightToLeft()
                onMoveRightRequested: triggerMoveLeftToRight()
                onMoveOtherRequested: triggerMoveRightToLeft()
                onViewSyncRequested: syncBrowserState(rightBrowser, leftBrowser)
                onSortKeyChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onSortOrderChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onDirsFirstChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onNameFilterChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onMinimumByteSizeChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onMaximumByteSizeChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onMinimumImageWidthChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onMaximumImageWidthChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onMinimumImageHeightChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onMaximumImageHeightChangedByUser: syncBrowserSettings(rightBrowser, leftBrowser)
                onGoUpRequested: syncGoUp(rightBrowser, leftBrowser)
                onRenameSucceeded: (message) => pushStatus(message)
            }

            }

            Connections {
                target: leftBrowser
                function onCopyFinished(result) {
                    handleCopyFinished(leftBrowser, result)
                }
                function onTrashFinished(result) {
                    handleTrashFinished(leftBrowser, result)
                }
                function onSelectedFileCountChanged() {
                    if (window.activeSelectionPane !== leftBrowser) {
                        return
                    }
                    leftPanel.selectedFileCount = leftBrowser.selectedFileCount
                }
                function onSelectedTotalBytesChanged() {
                    if (window.activeSelectionPane !== leftBrowser) {
                        return
                    }
                    leftPanel.selectedTotalBytes = leftBrowser.selectedTotalBytes
                }
            }

            Connections {
                target: rightBrowser
                function onCopyFinished(result) {
                    handleCopyFinished(rightBrowser, result)
                }
                function onTrashFinished(result) {
                    handleTrashFinished(rightBrowser, result)
                }
                function onSelectedFileCountChanged() {
                    if (window.activeSelectionPane !== rightBrowser) {
                        return
                    }
                    leftPanel.selectedFileCount = rightBrowser.selectedFileCount
                }
                function onSelectedTotalBytesChanged() {
                    if (window.activeSelectionPane !== rightBrowser) {
                        return
                    }
                    leftPanel.selectedTotalBytes = rightBrowser.selectedTotalBytes
                }
            }

            ImageCompare {
                id: comparePreview
                anchors.fill: parent
                visible: compareModel.enabled && leftPanel.syncPreviewEnabled
                panelBackground: window.panelBackground
                leftBrowser: leftBrowser
                rightBrowser: rightBrowser
                navigationBrowser: comparePreviewBrowser
                onCopyLeftRequested: triggerCopyRightToLeft()
                onCopyRightRequested: triggerCopyLeftToRight()
                onCloseRequested: {
                    leftPanel.syncPreviewEnabled = false
                    comparePreviewBrowser = null
                }
            }

    ImagePreview {
        id: singlePreview
        anchors.fill: parent
        visible: singlePreviewEnabled && !compareModel.enabled
        panelBackground: window.panelBackground
        browser: singlePreviewBrowser
        onCopyLeftRequested: triggerCopyRightToLeft()
        onCopyRightRequested: triggerCopyLeftToRight()
        onCloseRequested: singlePreviewEnabled = false
        onSaveSucceeded: (message) => pushStatus(message)
    }

            Item {
                anchors.fill: parent
                visible: compareModel.enabled && compareModel.loading
                    && !leftPanel.syncPreviewEnabled
                    && !singlePreviewEnabled

                Rectangle {
                    anchors.fill: parent
                    color: Qt.rgba(0, 0, 0, 0.25)
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: Theme.spaceSm

                    BusyIndicator {
                        running: true
                        width: 36
                        height: 36
                    }

                    Label {
                        text: qsTr("Comparing...")
                        opacity: 0.8
                    }
                }
            }
            }
        }

        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            Layout.minimumHeight: 28
            Layout.preferredHeight: 28
            Layout.maximumHeight: 28
            color: Theme.statusBarBackground
            radius: 6
            visible: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.statusBarPadding
                spacing: Theme.spaceSm

                Label {
                    Layout.fillWidth: true
                    text: currentError !== "" ? currentError : "Ready"
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                ProgressBar {
                    Layout.preferredWidth: statusBar.width / 3
                    Layout.minimumWidth: 120
                    Layout.fillHeight: true
                    value: copyProgress
                    opacity: copyInProgress ? 1.0 : 0.35
                    visible: true
                }

                ToolButton {
                    text: qsTr("Stop")
                    enabled: copyInProgress
                    visible: copyInProgress
                    onClicked: stopCopy()
                }
            }
        }
    }

    ConfirmDialog {
        id: confirmDialog
        parent: window.contentItem
        onTrashConfirmed: (sourcePane) => {
            if (sourcePane) {
                startTrashSelection(sourcePane)
            }
        }
        onDeleteConfirmed: (sourcePane) => {
            if (sourcePane) {
                startDeleteSelection(sourcePane)
            }
        }
        onMoveConfirmed: (sourcePane, targetPane) => {
            if (!sourcePane || !targetPane) {
                return
            }
            confirmDialog.close()
            startMove(sourcePane, targetPane)
        }
        onCopyConfirmed: (sourcePane, targetPane) => {
            if (!sourcePane || !targetPane) {
                return
            }
            confirmDialog.close()
            const sourcePaths = sourcePane.selectedPaths()
            startCopy(sourcePane, targetPane)
        }
    }

    ShortcutsDialog {
        id: shortcutsDialog
        parent: window.contentItem
    }
}
