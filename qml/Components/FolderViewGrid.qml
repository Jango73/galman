import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.15
import QtQuick.Window 2.15
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
    property real baseCellRatio: 0.25
    property int statusPending: 1
    property int statusIdentical: 2
    property int statusDifferent: 3
    property color statusIdenticalColor: Theme.statusIdentical
    property color statusDifferentColor: Theme.statusDifferent
    property Item tabFocusItem: null
    property Item backtabFocusItem: null
    property Item focusItem: grid
    readonly property int count: grid.count
    readonly property int currentIndex: grid.currentIndex
    readonly property real contentY: grid.contentY
    readonly property bool gridActiveFocus: grid.activeFocus
    readonly property int minimumPageRows: 1
    readonly property int pageRows: Math.max(minimumPageRows, Math.floor(grid.height / grid.cellHeight))
    property int contextMenuIndex: -1
    property string typeAheadBuffer: ""
    readonly property int selectedCount: root.browserModel ? root.browserModel.selectedPaths.length : 0
    signal viewSyncRequested()
    signal copyLeftRequested()
    signal copyRightRequested()
    signal moveLeftRequested()
    signal moveRightRequested()
    signal moveOtherRequested()
    signal copyOtherRequested()
    signal contentYUpdated(real value)
    signal currentIndexUpdated(int value)
    signal renameRequested(string path)
    signal trashRequested()
    signal deleteRequested()
    signal backgroundClicked()

    Timer {
        id: scrollRelockTimer
        interval: root.scrollRelockDelayMs
        repeat: false
        onTriggered: root.lockScroll()
    }

    Timer {
        id: typeAheadResetTimer
        interval: Theme.typeAheadResetMilliseconds
        repeat: false
        onTriggered: root.typeAheadBuffer = ""
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

    function selectIndex(index, modifiers) {
        if (!root.browserModel || index < 0) {
            return
        }
        if ((modifiers & Qt.ShiftModifier) !== 0 && root.anchorIndex >= 0) {
            const additive = (modifiers & Qt.ControlModifier) !== 0
            root.browserModel.setSelectionRange(root.anchorIndex, index, additive)
        } else if ((modifiers & Qt.ControlModifier) !== 0) {
            root.browserModel.select(index, true)
            root.anchorIndex = index
        } else {
            root.browserModel.select(index, false)
            root.anchorIndex = index
        }
        root.allowProgrammaticScroll()
        grid.currentIndex = index
        grid.positionViewAtIndex(index, GridView.Visible)
        Qt.callLater(root.lockScroll)
        grid.forceActiveFocus()
    }

    function prepareDragSelection(index, modifiers) {
        if (!root.browserModel || index < 0) {
            return
        }
        const selected = root.browserModel.isSelected ? root.browserModel.isSelected(index) : false
        if (selected) {
            return
        }
        const hasShift = (modifiers & Qt.ShiftModifier) !== 0
        const hasCtrl = (modifiers & Qt.ControlModifier) !== 0
        if (hasShift && root.anchorIndex >= 0) {
            root.browserModel.setSelectionRange(root.anchorIndex, index, hasCtrl)
        } else if (hasCtrl) {
            root.browserModel.select(index, true)
            root.anchorIndex = index
        } else {
            root.browserModel.select(index, false)
            root.anchorIndex = index
        }
        grid.currentIndex = index
    }

    function dragDataForIndex(index) {
        if (!root.browserModel) {
            return { urls: [], paths: [] }
        }
        let paths = root.browserModel.selectedPaths || []
        if (paths.length === 0 && index >= 0 && root.browserModel.pathForRow) {
            const path = root.browserModel.pathForRow(index)
            if (path) {
                paths = [path]
            }
        }
        const urls = []
        const pathList = []
        for (let i = 0; i < paths.length; i += 1) {
            const path = paths[i]
            if (path) {
                urls.push(Qt.resolvedUrl("file://" + path))
                pathList.push(path)
            }
        }
        return { urls: urls, paths: pathList }
    }

    function openContextMenuForIndex(index, parentItem, x, y) {
        if (!root.browserModel || index < 0) {
            return
        }
        contextMenuIndex = index
        const selected = root.browserModel.isSelected ? root.browserModel.isSelected(index) : false
        if (!selected) {
            root.browserModel.select(index, false)
            root.anchorIndex = index
            grid.currentIndex = index
        }
        const container = contextMenu.parent ? contextMenu.parent : root
        const point = parentItem.mapToItem(container, x, y)
        contextMenu.x = point.x
        contextMenu.y = point.y
        contextMenu.open()
    }

    Frame {
        anchors.fill: parent
        padding: 0
        background: FocusFrame {
            active: grid.activeFocus
        }

        GridView {
            id: grid
            anchors.fill: parent
            anchors.margins: Theme.spaceMd
            clip: true
            model: root.browserModel
            rightMargin: Theme.scrollBarThickness
            readonly property real availableWidth: Math.max(0, width - rightMargin)
            readonly property real baseCellSize: Math.max(
                root.minCellSize,
                Math.min(width, height) * root.baseCellRatio
            )
            readonly property int columnCount: Math.max(1, Math.floor(availableWidth / baseCellSize))
            cellWidth: Math.floor(availableWidth / columnCount)
            cellHeight: cellWidth
            keyNavigationWraps: true
            displayMarginBeginning: 120
            displayMarginEnd: 120
            activeFocusOnTab: true
            highlightFollowsCurrentItem: false
            highlight: null
            ScrollBar.vertical: ScrollBar {
                id: verticalScrollBar
                policy: ScrollBar.AsNeeded
                width: Theme.scrollBarThickness
                implicitWidth: Theme.scrollBarThickness
                z: 10
                onPressedChanged: {
                    if (pressed) {
                        root.unlockScroll()
                    } else {
                        root.lockScroll()
                    }
                }
            }
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
                const hasShift = (event.modifiers & Qt.ShiftModifier) !== 0
                const hasAlt = (event.modifiers & Qt.AltModifier) !== 0
                const hasMeta = (event.modifiers & Qt.MetaModifier) !== 0
                const hasCtrl = (event.modifiers & Qt.ControlModifier) !== 0

                if (hasShift && !hasAlt && !hasMeta && !hasCtrl) {
                    if (event.key === Qt.Key_Left) {
                        root.moveLeftRequested()
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Right) {
                        root.moveRightRequested()
                        event.accepted = true
                        return
                    }
                }

                if (hasCtrl) {
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

                if (!hasAlt && !hasMeta && !hasCtrl && event.text && event.text.length === 1) {
                    const typed = event.text.toLowerCase()
                    if (typed >= "a" && typed <= "z") {
                        root.typeAheadBuffer = root.typeAheadBuffer + typed
                        typeAheadResetTimer.restart()
                        if (root.browserModel && root.browserModel.rowForPrefix) {
                            const startRow = grid.currentIndex >= 0 ? grid.currentIndex + 1 : 0
                            let row = root.browserModel.rowForPrefix(root.typeAheadBuffer, startRow)
                            if (row < 0 && root.typeAheadBuffer.length > 1) {
                                root.typeAheadBuffer = typed
                                typeAheadResetTimer.restart()
                                row = root.browserModel.rowForPrefix(typed, startRow)
                            }
                            if (row >= 0) {
                                root.browserModel.select(row, false)
                                root.anchorIndex = row
                                root.allowProgrammaticScroll()
                                grid.currentIndex = row
                                grid.positionViewAtIndex(row, GridView.Visible)
                                Qt.callLater(root.lockScroll)
                            }
                        }
                        event.accepted = true
                        return
                    }
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
                } else if (event.key === Qt.Key_PageUp) {
                    const cols = Math.max(1, Math.floor(grid.width / grid.cellWidth))
                    next = Math.max(0, current - (cols * root.pageRows))
                } else if (event.key === Qt.Key_PageDown) {
                    const cols = Math.max(1, Math.floor(grid.width / grid.cellWidth))
                    next = Math.min(grid.count - 1, current + (cols * root.pageRows))
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

            TapHandler {
                acceptedButtons: Qt.LeftButton
                onTapped: (eventPoint) => {
                    const idx = grid.indexAt(
                        eventPoint.position.x + grid.contentX,
                        eventPoint.position.y + grid.contentY
                    )
                    if (idx < 0) {
                        root.backgroundClicked()
                        grid.forceActiveFocus()
                    }
                }
            }

            MouseArea {
                id: emptyGridFocusArea
                anchors.fill: parent
                visible: grid.count === 0
                enabled: visible
                acceptedButtons: Qt.LeftButton
                z: 2
                onPressed: (mouse) => {
                    root.backgroundClicked()
                    grid.forceActiveFocus()
                    mouse.accepted = true
                }
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
                    otherSidePath: model.otherSidePath ? model.otherSidePath : ""
                    fileName: model.fileName
                    modifiedText: model.isGhost
                        ? ""
                        : (model.modified ? Qt.formatDateTime(model.modified, Qt.DefaultLocaleShortDate) : "")
                    compareStatus: model.compareStatus
                    statusPending: root.statusPending
                    statusIdentical: root.statusIdentical
                    statusDifferent: root.statusDifferent
                    statusIdenticalColor: root.statusIdenticalColor
                    statusDifferentColor: root.statusDifferentColor
                }

                MouseArea {
                    id: itemMouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    hoverEnabled: false
                    propagateComposedEvents: true

                    property real startX: 0
                    property real startY: 0
                    property bool didDrag: false
                    property var dragMimeData: ({})

                    Drag.dragType: Drag.Automatic
                    Drag.mimeData: dragMimeData
                    Drag.supportedActions: Qt.CopyAction
                    Drag.proposedAction: Qt.CopyAction
                    Drag.onDragFinished: {
                        Drag.active = false
                        dragMimeData = ({})
                    }

                    function startExternalDrag(modifiers) {
                        root.prepareDragSelection(index, modifiers)
                        const data = root.dragDataForIndex(index)
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

                    onPressed: (mouse) => {
                        grid.forceActiveFocus()
                        startX = mouseX
                        startY = mouseY
                        didDrag = false
                        if (mouse.button === Qt.RightButton) {
                            root.openContextMenuForIndex(index, row, mouse.x, mouse.y)
                            mouse.accepted = true
                        }
                    }

                    onPositionChanged: (mouse) => {
                        if (!pressed || Drag.active) {
                            return
                        }
                        if ((mouse.buttons & Qt.RightButton) !== 0) {
                            return
                        }
                        const dx = Math.abs(mouseX - startX)
                        const dy = Math.abs(mouseY - startY)
                        if (dx > 6 || dy > 6) {
                            startExternalDrag(mouse.modifiers)
                        }
                    }

                    onClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            return
                        }
                        if (didDrag) {
                            didDrag = false
                            return
                        }
                        root.selectIndex(index, mouse.modifiers)
                    }

                    onDoubleClicked: (mouse) => {
                        if (mouse.button === Qt.RightButton) {
                            return
                        }
                        if (didDrag) {
                            didDrag = false
                            return
                        }
                        grid.forceActiveFocus()
                        if (root.browserModel) {
                            root.browserModel.activate(index)
                        }
                    }
                }
            }
        }

        Menu {
            id: contextMenu
            parent: Window.window ? Window.window.contentItem : root
            property real menuContentWidth: implicitWidth

            function updateMenuWidth() {
                let widest = implicitWidth
                for (let i = 0; i < count; i += 1) {
                    const item = itemAt(i)
                    if (item) {
                        widest = Math.max(widest, item.implicitWidth)
                    }
                }
                menuContentWidth = widest
            }

            width: menuContentWidth
            onAboutToShow: updateMenuWidth()
            Instantiator {
                id: renameFactory
                active: root.selectedCount === 1
                delegate: MenuItem {
                    text: qsTr("Rename")
                    onTriggered: {
                        if (!root.browserModel || !root.browserModel.pathForRow) {
                            return
                        }
                        const path = root.browserModel.pathForRow(root.contextMenuIndex)
                        if (path) {
                            root.renameRequested(path)
                        }
                    }
                }
                onObjectAdded: (index, object) => {
                    contextMenu.addItem(object)
                    contextMenu.updateMenuWidth()
                }
                onObjectRemoved: (index, object) => {
                    contextMenu.removeItem(object)
                    contextMenu.updateMenuWidth()
                }
            }
            Instantiator {
                id: copyOtherFactory
                active: root.selectedCount > 0
                delegate: MenuItem {
                    text: qsTr("Copy to other pane")
                    onTriggered: root.copyOtherRequested()
                }
                onObjectAdded: (index, object) => {
                    contextMenu.addItem(object)
                    contextMenu.updateMenuWidth()
                }
                onObjectRemoved: (index, object) => {
                    contextMenu.removeItem(object)
                    contextMenu.updateMenuWidth()
                }
            }
            Instantiator {
                id: moveOtherFactory
                active: root.selectedCount > 0
                delegate: MenuItem {
                    text: qsTr("Move to other pane")
                    onTriggered: root.moveOtherRequested()
                }
                onObjectAdded: (index, object) => {
                    contextMenu.addItem(object)
                    contextMenu.updateMenuWidth()
                }
                onObjectRemoved: (index, object) => {
                    contextMenu.removeItem(object)
                    contextMenu.updateMenuWidth()
                }
            }
            Instantiator {
                id: trashFactory
                active: root.selectedCount > 0
                delegate: MenuItem {
                    text: qsTr("Move to trash")
                    onTriggered: root.trashRequested()
                }
                onObjectAdded: (index, object) => {
                    contextMenu.addItem(object)
                    contextMenu.updateMenuWidth()
                }
                onObjectRemoved: (index, object) => {
                    contextMenu.removeItem(object)
                    contextMenu.updateMenuWidth()
                }
            }
            Instantiator {
                id: deleteFactory
                active: root.selectedCount > 0
                delegate: MenuItem {
                    text: qsTr("Delete permanently")
                    onTriggered: root.deleteRequested()
                }
                onObjectAdded: (index, object) => {
                    contextMenu.addItem(object)
                    contextMenu.updateMenuWidth()
                }
                onObjectRemoved: (index, object) => {
                    contextMenu.removeItem(object)
                    contextMenu.updateMenuWidth()
                }
            }
        }

    }

    Component.onCompleted: {
        lockScroll()
    }
}
