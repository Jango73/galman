import QtQuick 2.15
import Galman 1.0

import "."

QtObject {
    id: root
    property string outputPath: ""
    readonly property string currentFolder: folderPathFromPath(outputPath)
    readonly property string selectedMediaPath: outputModel ? outputModel.selectedPaths[0] || "" : ""
    readonly property bool selectedIsImage: outputModel ? outputModel.selectedIsImage : false
    readonly property bool selectedIsVideo: outputModel ? outputModel.selectedIsVideo : false
    readonly property int selectedCompareStatus: 0
    readonly property bool selectedGhost: false
    readonly property bool selectedIsNewer: false
    readonly property int statusPending: 1
    readonly property int statusIdentical: 2
    readonly property int statusDifferent: 3
    readonly property color statusIdenticalColor: Theme.statusIdentical
    readonly property color statusDifferentColor: Theme.statusDifferent
    readonly property int syncRetryInterval: 500
    readonly property int syncMaxAttempts: 20
    property string pendingOutputPath: ""
    property int syncAttempts: 0
    property int pendingDeletionRow: -1
    readonly property int invalidRow: -1
    signal selectionRestoredAfterRemoval

    property var outputModel: FolderBrowserModel {
    }

    property var syncTimer: Timer {
        interval: root.syncRetryInterval
        repeat: true
        running: false
        onTriggered: root.trySelectPendingOutput()
    }

    property var loadingWatcher: Connections {
        target: root.outputModel
        function onLoadingChanged() {
            if (!root.outputModel.loading) {
                root.restoreSelectionAfterRemoval()
            }
        }
    }

    onOutputPathChanged: syncToOutput(outputPath)
    onCurrentFolderChanged: syncToOutput(outputPath)

    function folderPathFromPath(path) {
        const normalized = String(path || "").replace(/\\/g, "/")
        const index = normalized.lastIndexOf("/")
        return index >= 0 ? normalized.slice(0, index) : ""
    }

    function syncToOutput(path) {
        pendingOutputPath = String(path || "")
        syncAttempts = 0
        if (pendingOutputPath === "") {
            syncTimer.stop()
            return
        }
        if (currentFolder !== "" && outputModel.rootPath !== currentFolder) {
            outputModel.rootPath = currentFolder
        }
        if (!trySelectPendingOutput()) {
            syncTimer.start()
        }
    }

    function trySelectPendingOutput() {
        if (pendingOutputPath === "") {
            syncTimer.stop()
            return true
        }
        if (selectPath(pendingOutputPath)) {
            pendingOutputPath = ""
            syncAttempts = 0
            syncTimer.stop()
            return true
        }
        syncAttempts += 1
        if (syncAttempts >= syncMaxAttempts) {
            syncTimer.stop()
        }
        return false
    }

    function rowForPath(path) {
        if (!outputModel || !path) {
            return -1
        }
        const count = outputModel.rowCount()
        for (let row = 0; row < count; row += 1) {
            if (outputModel.pathForRow(row) === path) {
                return row
            }
        }
        return -1
    }

    function selectPath(path) {
        const row = rowForPath(path)
        if (row < 0) {
            return false
        }
        outputModel.select(row, false)
        return true
    }

    function currentSelectedRow() {
        if (!outputModel) {
            return -1
        }
        const rows = outputModel.selectedRows()
        if (rows && rows.length > 0) {
            return rows[0]
        }
        return rowForPath(selectedMediaPath)
    }

    function selectAdjacentMedia(step) {
        if (!outputModel) {
            return
        }
        const count = outputModel.rowCount()
        if (count === 0) {
            return
        }
        let index = currentSelectedRow()
        if (index < 0) {
            index = step > 0 ? -1 : count
        }
        for (let visited = 0; visited < count; visited += 1) {
            index += step
            if (index < 0 || index >= count) {
                return
            }
            if (outputModel.isImage(index) || outputModel.isVideo(index)) {
                outputModel.select(index, false)
                return
            }
        }
    }

    function selectBoundaryMedia(first) {
        if (!outputModel) {
            return
        }
        const count = outputModel.rowCount()
        if (count === 0) {
            return
        }
        if (first) {
            for (let row = 0; row < count; row += 1) {
                if (outputModel.isImage(row) || outputModel.isVideo(row)) {
                    outputModel.select(row, false)
                    return
                }
            }
            return
        }
        for (let row = count - 1; row >= 0; row -= 1) {
            if (outputModel.isImage(row) || outputModel.isVideo(row)) {
                outputModel.select(row, false)
                return
            }
        }
    }

    function selectNeighborMedia(row) {
        if (!outputModel) {
            return false
        }
        const count = outputModel.rowCount()
        if (count === 0) {
            return false
        }
        const clamped = Math.max(0, Math.min(row, count - 1))
        for (let next = clamped; next < count; next += 1) {
            if (outputModel.isImage(next) || outputModel.isVideo(next)) {
                outputModel.select(next, false)
                return true
            }
        }
        for (let previous = clamped - 1; previous >= 0; previous -= 1) {
            if (outputModel.isImage(previous) || outputModel.isVideo(previous)) {
                outputModel.select(previous, false)
                return true
            }
        }
        return false
    }

    function restoreSelectionAfterRemoval() {
        if (pendingDeletionRow === invalidRow) {
            return
        }
        const row = pendingDeletionRow
        pendingDeletionRow = invalidRow
        if (selectedPaths().length > 0) {
            return
        }
        if (selectNeighborMedia(row)) {
            selectionRestoredAfterRemoval()
        }
    }

    function selectedPaths() {
        return outputModel ? outputModel.selectedPaths : []
    }

    function trashSelected() {
        if (!outputModel) {
            return { "ok": false, "error": "No model" }
        }
        if (outputModel.startMoveSelectedToTrash) {
            pendingDeletionRow = currentSelectedRow()
            outputModel.startMoveSelectedToTrash()
            return { "ok": true }
        }
        if (outputModel.moveSelectedToTrash) {
            pendingDeletionRow = currentSelectedRow()
            return outputModel.moveSelectedToTrash()
        }
        return { "ok": false, "error": "Trash not supported" }
    }

    function deleteSelectedPermanently() {
        if (!outputModel) {
            return { "ok": false, "error": "No model" }
        }
        if (outputModel.startDeleteSelectedPermanently) {
            pendingDeletionRow = currentSelectedRow()
            outputModel.startDeleteSelectedPermanently()
            return { "ok": true }
        }
        if (outputModel.deleteSelectedPermanently) {
            pendingDeletionRow = currentSelectedRow()
            return outputModel.deleteSelectedPermanently()
        }
        return { "ok": false, "error": "Delete not supported" }
    }

    function removalSourcePane() {
        return outputModel
    }
}
