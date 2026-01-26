import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import "."
import Galman 1.0

Item {
    id: root
    property var browserModel: null
    property int minCellSize: 150
    property int anchorIndex: -1
    property bool syncEnabled: true
    property bool scrollLockEnabled: true
    property bool scrollLockActive: true
    property bool scrollLockOverride: false
    property bool applyingScrollLock: false
    property real lockedContentY: 0
    property int scrollRelockDelayMs: 150
    property int statusPending: 1
    property int statusIdentical: 2
    property int statusDifferent: 3
    property color statusIdenticalColor: Theme.statusIdentical
    property color statusDifferentColor: Theme.statusDifferent
    property Item tabFocusItem: null
    property Item backtabFocusItem: null
    readonly property int count: grid.count
    readonly property int currentIndex: grid.currentIndex
    readonly property real contentY: grid.contentY
    readonly property bool gridActiveFocus: grid.activeFocus
    signal viewSyncRequested()
    signal copyLeftRequested()
    signal copyRightRequested()
    signal contentYUpdated(real value)
    signal currentIndexUpdated(int value)

    Timer {
        id: scrollRelockTimer
        interval: root.scrollRelockDelayMs
        repeat: false
        onTriggered: root.lockScroll()
    }

    function lockScroll() {
        if (!scrollLockEnabled) {
            return
        }
        lockedContentY = grid.contentY
        scrollLockActive = true
        scrollLockOverride = false
    }

    function unlockScroll() {
        if (!scrollLockEnabled) {
            return
        }
        scrollLockActive = false
    }

    function allowProgrammaticScroll() {
        if (!scrollLockEnabled) {
            return
        }
        scrollLockActive = false
        scrollLockOverride = true
    }

    function positionViewAtIndex(index) {
        allowProgrammaticScroll()
        grid.positionViewAtIndex(index, GridView.Visible)
        Qt.callLater(lockScroll)
    }

    function setCurrentIndex(index) {
        allowProgrammaticScroll()
        grid.currentIndex = index
        Qt.callLater(lockScroll)
    }

    function setContentY(value) {
        allowProgrammaticScroll()
        grid.contentY = value
        Qt.callLater(lockScroll)
    }

    Frame {
        anchors.fill: parent
        padding: 0
        background: Rectangle {
            color: "transparent"
            radius: 0
            border.color: grid.activeFocus ? Material.accent : "transparent"
            border.width: grid.activeFocus ? 2 : 0
        }

        GridView {
            id: grid
            anchors.fill: parent
            anchors.margins: Theme.spaceMd
            clip: true
            model: root.browserModel
            cellWidth: Math.floor(width / Math.max(1, Math.floor(width / root.minCellSize)))
            cellHeight: cellWidth
            keyNavigationWraps: true
            displayMarginBeginning: 120
            displayMarginEnd: 120
            activeFocusOnTab: true
            highlightFollowsCurrentItem: false
            highlight: null
            onContentYChanged: {
                if (root.scrollLockEnabled && root.scrollLockActive && !root.applyingScrollLock) {
                    if (Math.abs(grid.contentY - root.lockedContentY) > 0.5) {
                        root.applyingScrollLock = true
                        grid.contentY = root.lockedContentY
                        root.applyingScrollLock = false
                        return
                    }
                }
                root.contentYUpdated(grid.contentY)
                if (root.syncEnabled) {
                    root.viewSyncRequested()
                }
            }
            onMovementStarted: {
                root.unlockScroll()
                scrollRelockTimer.stop()
            }
            onMovementEnded: {
                root.lockScroll()
            }
            onCurrentIndexChanged: {
                root.currentIndexUpdated(grid.currentIndex)
                if (root.syncEnabled) {
                    root.viewSyncRequested()
                }
            }
            Keys.onPressed: (event) => {
                if ((event.modifiers & Qt.ControlModifier) !== 0) {
                    if (event.key === Qt.Key_Left) {
                        root.copyLeftRequested()
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Right) {
                        root.copyRightRequested()
                        event.accepted = true
                        return
                    }
                }
                if (event.key === Qt.Key_Tab) {
                    if (root.tabFocusItem) {
                        root.tabFocusItem.forceActiveFocus()
                        event.accepted = true
                        return
                    }
                }
                if (event.key === Qt.Key_Backtab) {
                    if (root.backtabFocusItem) {
                        root.backtabFocusItem.forceActiveFocus()
                        event.accepted = true
                        return
                    }
                }
                if (grid.count === 0) {
                    event.accepted = false
                    return
                }

                const current = grid.currentIndex < 0 ? 0 : grid.currentIndex
                let next = current

                if (event.key === Qt.Key_Left) {
                    next = Math.max(0, current - 1)
                } else if (event.key === Qt.Key_Right) {
                    next = Math.min(grid.count - 1, current + 1)
                } else if (event.key === Qt.Key_Up) {
                    const cols = Math.max(1, Math.floor(grid.width / grid.cellWidth))
                    next = Math.max(0, current - cols)
                } else if (event.key === Qt.Key_Down) {
                    const cols = Math.max(1, Math.floor(grid.width / grid.cellWidth))
                    next = Math.min(grid.count - 1, current + cols)
                } else if (event.key === Qt.Key_Home) {
                    next = 0
                } else if (event.key === Qt.Key_End) {
                    next = grid.count - 1
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    if (root.browserModel) {
                        root.browserModel.activate(current)
                    }
                    event.accepted = true
                    return
                } else if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
                    if (root.browserModel) {
                        if (root.browserModel.allSelected()) {
                            root.browserModel.clearSelection()
                        } else {
                            root.browserModel.setSelectionRange(0, grid.count - 1, false)
                        }
                    }
                    event.accepted = true
                    return
                } else if (event.key === Qt.Key_Space) {
                    if (root.browserModel) {
                        if (event.modifiers & Qt.ControlModifier) {
                            root.browserModel.select(current, true)
                        } else {
                            root.browserModel.select(current, false)
                            root.anchorIndex = current
                        }
                    }
                    event.accepted = true
                    return
                } else {
                    event.accepted = false
                    return
                }

                const hasShift = (event.modifiers & Qt.ShiftModifier) !== 0
                const hasCtrl = (event.modifiers & Qt.ControlModifier) !== 0

                if (root.browserModel) {
                    if (hasShift) {
                        if (root.anchorIndex < 0) {
                            root.anchorIndex = current
                        }
                        root.browserModel.setSelectionRange(root.anchorIndex, next, hasCtrl)
                    } else if (!hasCtrl) {
                        root.browserModel.select(next, false)
                        root.anchorIndex = next
                    }
                }

                root.allowProgrammaticScroll()
                grid.currentIndex = next
                grid.positionViewAtIndex(next, GridView.Visible)
                Qt.callLater(root.lockScroll)
                event.accepted = true
            }

            delegate: Item {
                id: row
                width: GridView.view ? GridView.view.cellWidth : 0
                height: GridView.view ? GridView.view.cellHeight : 0

                FolderItem {
                    anchors.fill: parent
                    selected: model.selected
                    isImage: model.isImage
                    isDir: model.isDir
                    isGhost: model.isGhost
                    filePath: model.filePath
                    fileName: model.fileName
                    modifiedText: model.modified ? Qt.formatDateTime(model.modified, Qt.DefaultLocaleShortDate) : ""
                    compareStatus: model.compareStatus
                    statusPending: root.statusPending
                    statusIdentical: root.statusIdentical
                    statusDifferent: root.statusDifferent
                    statusIdenticalColor: root.statusIdenticalColor
                    statusDifferentColor: root.statusDifferentColor
                }
            }
        }

        Rectangle {
            id: rubberBand
            color: Qt.rgba(Material.accent.r, Material.accent.g, Material.accent.b, 0.18)
            border.color: Material.accent
            border.width: 1
            visible: false
            z: 3
        }

        MouseArea {
            id: rubberBandArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            propagateComposedEvents: true
            z: 4

            property real startX: 0
            property real startY: 0
            property bool dragging: false
            property bool didDrag: false
            property int pressIndex: -1
            property var dragMimeData: ({})

            Drag.dragType: Drag.Automatic
            Drag.mimeData: dragMimeData
            Drag.supportedActions: Qt.CopyAction
            Drag.proposedAction: Qt.CopyAction
            Drag.onDragFinished: {
                const urls = dragMimeData["text/uri-list"] || []
                const paths = dragMimeData["text/plain"] || ""
                Drag.active = false
                dragMimeData = ({})
            }

            function currentRect() {
                const x1 = Math.min(startX, mouseX)
                const y1 = Math.min(startY, mouseY)
                const x2 = Math.max(startX, mouseX)
                const y2 = Math.max(startY, mouseY)
                return { x: x1, y: y1, w: x2 - x1, h: y2 - y1 }
            }

            function applySelection(additive) {
                if (!root.browserModel) {
                    return
                }
                const rect = currentRect()
                const left = rect.x + grid.contentX
                const top = rect.y + grid.contentY
                const right = left + rect.w
                const bottom = top + rect.h

                const columns = Math.max(1, Math.floor(grid.width / grid.cellWidth))
                const count = grid.count
                const hits = []

                for (let i = 0; i < count; i++) {
                    const col = i % columns
                    const row = Math.floor(i / columns)
                    const x = col * grid.cellWidth
                    const y = row * grid.cellHeight
                    const x2 = x + grid.cellWidth
                    const y2 = y + grid.cellHeight

                    const intersect = !(x2 < left || x > right || y2 < top || y > bottom)
                    if (intersect) {
                        hits.push(i)
                    }
                }

                root.browserModel.setSelection(hits, additive)
                if (hits.length > 0) {
                    grid.currentIndex = hits[0]
                    root.anchorIndex = hits[0]
                }
            }

            function prepareDragSelection(modifiers) {
                if (!root.browserModel || pressIndex < 0) {
                    return
                }
                const selected = root.browserModel.isSelected
                    ? root.browserModel.isSelected(pressIndex)
                    : false
                if (selected) {
                    return
                }
                const hasShift = (modifiers & Qt.ShiftModifier) !== 0
                const hasCtrl = (modifiers & Qt.ControlModifier) !== 0
                if (hasShift && root.anchorIndex >= 0) {
                    root.browserModel.setSelectionRange(root.anchorIndex, pressIndex, hasCtrl)
                } else if (hasCtrl) {
                    root.browserModel.select(pressIndex, true)
                    root.anchorIndex = pressIndex
                } else {
                    root.browserModel.select(pressIndex, false)
                    root.anchorIndex = pressIndex
                }
                grid.currentIndex = pressIndex
            }

            function dragData() {
                if (!root.browserModel) {
                    return { urls: [], paths: [] }
                }
                let paths = root.browserModel.selectedPaths || []
                if (paths.length === 0 && pressIndex >= 0 && root.browserModel.pathForRow) {
                    const path = root.browserModel.pathForRow(pressIndex)
                    if (path) {
                        paths = [path]
                    }
                }
                const urls = []
                const pathList = []
                for (let i = 0; i < paths.length; i++) {
                    const path = paths[i]
                    if (path) {
                        const url = Qt.resolvedUrl("file://" + path)
                        urls.push(url)
                        pathList.push(path)
                    }
                }
                return { urls: urls, paths: pathList }
            }

            function startExternalDrag(modifiers) {
                prepareDragSelection(modifiers)
                const data = dragData()
                if (data.urls.length === 0) {
                    return
                }
                let uriList = data.urls.join("\r\n")
                if (uriList !== "") {
                    uriList += "\r\n"
                }
                Drag.hotSpot.x = mouseX
                Drag.hotSpot.y = mouseY
                dragMimeData = {
                    "text/uri-list": uriList,
                    "text/plain": data.paths.join("\n")
                }
                Drag.active = true
                didDrag = true
            }

            onPressed: {
                grid.forceActiveFocus()
                startX = mouseX
                startY = mouseY
                dragging = false
                didDrag = false
                pressIndex = grid.indexAt(mouseX + grid.contentX, mouseY + grid.contentY)
            }

            onPositionChanged: (mouse) => {
                if (!pressed) {
                    return
                }
                if (Drag.active) {
                    return
                }
                const dx = Math.abs(mouseX - startX)
                const dy = Math.abs(mouseY - startY)
                if (!dragging && (dx > 6 || dy > 6)) {
                    if (pressIndex >= 0) {
                        startExternalDrag(mouse.modifiers)
                        return
                    }
                    dragging = true
                    didDrag = true
                    rubberBand.visible = true
                }

                if (dragging) {
                    const rect = currentRect()
                    rubberBand.x = rect.x
                    rubberBand.y = rect.y
                    rubberBand.width = rect.w
                    rubberBand.height = rect.h
                }
            }

            onReleased: (mouse) => {
                if (Drag.active) {
                    return
                }
                if (dragging) {
                    const additive = (mouse.modifiers & Qt.ControlModifier) !== 0
                    applySelection(additive)
                    rubberBand.visible = false
                    dragging = false
                    didDrag = true
                    return
                }
            }

            onClicked: (mouse) => {
                grid.forceActiveFocus()
                if (didDrag) {
                    didDrag = false
                    return
                }
                if (!root.browserModel) {
                    return
                }
                const idx = grid.indexAt(mouseX + grid.contentX, mouseY + grid.contentY)
                if (idx >= 0) {
                    if ((mouse.modifiers & Qt.ShiftModifier) !== 0 && root.anchorIndex >= 0) {
                        const additive = (mouse.modifiers & Qt.ControlModifier) !== 0
                        root.browserModel.setSelectionRange(root.anchorIndex, idx, additive)
                    } else if ((mouse.modifiers & Qt.ControlModifier) !== 0) {
                        root.browserModel.select(idx, true)
                        root.anchorIndex = idx
                    } else {
                        root.browserModel.select(idx, false)
                        root.anchorIndex = idx
                    }
                    root.allowProgrammaticScroll()
                    grid.currentIndex = idx
                    grid.positionViewAtIndex(idx, GridView.Visible)
                    Qt.callLater(root.lockScroll)
                    grid.forceActiveFocus()
                }
            }

            onDoubleClicked: (mouse) => {
                grid.forceActiveFocus()
                if (didDrag) {
                    didDrag = false
                    return
                }
                if (!root.browserModel) {
                    return
                }
                const idx = grid.indexAt(mouseX + grid.contentX, mouseY + grid.contentY)
                if (idx >= 0) {
                    root.browserModel.activate(idx)
                }
            }
        }
    }

    Component.onCompleted: {
        lockScroll()
    }
}
