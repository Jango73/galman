import QtMultimedia
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import Galman 1.0

import "."

Pane {
    id: root
    property color panelBackground: Theme.panelBackground
    property string mediaPath: ""
    property bool mediaIsVideo: false
    property string emptyText: "No media selected"
    property int compareStatus: 0
    property bool ghost: false
    property int statusPending: 1
    property int statusIdentical: 2
    property int statusDifferent: 3
    property color statusIdenticalColor: Theme.statusIdentical
    property color statusDifferentColor: Theme.statusDifferent
    property bool isNewer: false
    property bool useAdjustments: false
    property real bloomValue: 0.0
    property real bloomRadiusPercent: 0.12
    property real brightnessValue: 0.0
    property real contrastValue: 0.0
    readonly property real opacityVisible: 1.0
    readonly property real opacityHidden: 0.0
    readonly property real centerFactor: 0.5
    property int rotation: 0
    property int reloadToken: 0
    property int invalidIndex: -1
    property string reloadQueryKey: "reload"
    property string querySeparator: "?"
    property string queryJoiner: "&"
    readonly property string resolvedImageSource: {
        if (root.mediaPath === "" || root.mediaIsVideo) {
            return ""
        }
        const separator = root.mediaPath.indexOf(root.querySeparator) === root.invalidIndex
            ? root.querySeparator
            : root.queryJoiner
        return root.mediaPath + separator + root.reloadQueryKey + "=" + root.reloadToken
    }
    readonly property bool showingImage: root.mediaPath !== "" && !root.mediaIsVideo
    readonly property bool showingVideo: root.mediaPath !== "" && root.mediaIsVideo
    readonly property bool videoPlaybackActive: root.visible && root.showingVideo
    readonly property real paintedWidth: imageItem.paintedWidth
    readonly property real paintedHeight: imageItem.paintedHeight
    readonly property real paintedHorizontalOffset: (imageItem.width - paintedWidth) * centerFactor
    readonly property real paintedVerticalOffset: (imageItem.height - paintedHeight) * centerFactor
    property real zoomLevel: 1.0
    property real panOffsetX: 0.0
    property real panOffsetY: 0.0
    readonly property bool isZoomed: zoomLevel > 1.001
    readonly property real zoomLevelForHundredPercent: {
        if (!root.showingImage || root.paintedWidth <= 0) {
            return 1.0
        }
        return imageItem.sourceSize.width / root.paintedWidth
    }

    padding: Theme.panelPadding
    Material.background: panelBackground

    onMediaPathChanged: {
        syncVideoPlayback()
        resetZoom()
    }
    onMediaIsVideoChanged: syncVideoPlayback()
    onVisibleChanged: syncVideoPlayback()
    onVideoPlaybackActiveChanged: syncVideoPlayback()
    onRotationChanged: resetZoom()

    function syncVideoPlayback() {
        if (root.videoPlaybackActive) {
            mediaPlayer.play()
            return
        }
        mediaPlayer.stop()
    }

    function resetZoom() {
        root.zoomLevel = 1.0
        root.panOffsetX = 0.0
        root.panOffsetY = 0.0
    }

    function zoomToHundredPercent() {
        const target = Math.min(root.zoomLevelForHundredPercent, Theme.zoomMaxLevel)
        const factor = target / root.zoomLevel
        zoomAtPoint(factor, contentItem.width / 2, contentItem.height / 2)
    }

    function zoomIn() {
        zoomAtPoint(Theme.zoomStepFactor, contentItem.width / 2, contentItem.height / 2)
    }

    function zoomOut() {
        zoomAtPoint(1.0 / Theme.zoomStepFactor, contentItem.width / 2, contentItem.height / 2)
    }

    function zoomAtPoint(factor, localX, localY) {
        const newZoom = Math.min(Math.max(root.zoomLevel * factor, Theme.zoomMinLevel), Theme.zoomMaxLevel)
        const actualFactor = newZoom / root.zoomLevel
        if (Math.abs(actualFactor - 1.0) < 0.001) {
            return
        }
        const centerX = contentItem.width / 2 + root.panOffsetX
        const centerY = contentItem.height / 2 + root.panOffsetY
        root.panOffsetX += (localX - centerX) * (1 - actualFactor)
        root.panOffsetY += (localY - centerY) * (1 - actualFactor)
        root.zoomLevel = newZoom
        clampPan()
    }

    function clampPan() {
        if (!root.showingImage) {
            return
        }
        const viewportWidth = contentItem.width
        const viewportHeight = contentItem.height
        const scaledWidth = root.paintedWidth * root.zoomLevel
        const scaledHeight = root.paintedHeight * root.zoomLevel
        const maxPanX = Math.max(0, (scaledWidth - viewportWidth) / 2)
        const maxPanY = Math.max(0, (scaledHeight - viewportHeight) / 2)
        root.panOffsetX = Math.max(-maxPanX, Math.min(maxPanX, root.panOffsetX))
        root.panOffsetY = Math.max(-maxPanY, Math.min(maxPanY, root.panOffsetY))
    }

    Item {
        id: contentItem
        anchors.fill: parent

        StatusBadge {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: Theme.spaceMd
            status: root.compareStatus
            ghost: root.ghost
            statusPending: root.statusPending
            statusIdentical: root.statusIdentical
            statusDifferent: root.statusDifferent
            statusIdenticalColor: root.statusIdenticalColor
            statusDifferentColor: root.statusDifferentColor
            z: 2
        }

        Canvas {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: Theme.spaceMd
            anchors.leftMargin: Theme.spaceMd + 12 + Theme.spaceXs
            width: 12
            height: 12
            visible: root.isNewer && !root.ghost
            z: 2
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                const cx = width / 2
                const cy = height / 2
                const r = width / 2 - 1
                ctx.strokeStyle = Theme.newerIndicator
                ctx.lineWidth = 1.5
                ctx.beginPath()
                ctx.arc(cx, cy, r, 0, Math.PI * 2)
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(cx, cy)
                ctx.lineTo(cx, cy - r * 0.55)
                ctx.stroke()
                ctx.beginPath()
                ctx.moveTo(cx, cy)
                ctx.lineTo(cx + r * 0.45, cy)
                ctx.stroke()
            }
        }

        Item {
            id: imageContainer
            x: (contentItem.width - width) / 2 + root.panOffsetX
            y: (contentItem.height - height) / 2 + root.panOffsetY
            readonly property bool isRotated: root.rotation % 180 !== 0
            width: isRotated ? parent.height : parent.width
            height: isRotated ? parent.width : parent.height
            scale: root.zoomLevel
            transformOrigin: Item.Center
            transform: Rotation {
                angle: root.rotation
                origin.x: imageContainer.width / 2
                origin.y: imageContainer.height / 2
            }

            Image {
                id: imageItem
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                cache: false
                source: root.resolvedImageSource
                asynchronous: true
                visible: root.showingImage
            }

            ImageAdjustmentsEffect {
                anchors.fill: parent
                sourceItem: imageItem
                active: root.useAdjustments && root.showingImage
                bloomValue: root.bloomValue
                bloomRadiusPercent: root.bloomRadiusPercent
                brightnessValue: root.brightnessValue
                contrastValue: root.contrastValue
            }
        }

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
            fillMode: VideoOutput.PreserveAspectFit
            visible: root.showingVideo
        }

        MediaPlayer {
            id: mediaPlayer
            source: root.videoPlaybackActive ? root.mediaPath : ""
            videoOutput: videoOutput
            audioOutput: audioOutput
        }

        AudioOutput {
            id: audioOutput
        }

        Label {
            anchors.centerIn: parent
            text: root.mediaPath === "" ? root.emptyText : ""
            opacity: 0.7
        }

        MouseArea {
            id: zoomPanArea
            anchors.fill: parent
            z: 1
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
            hoverEnabled: true
            cursorShape: root.showingImage && root.isZoomed
                ? (pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor)
                : Qt.ArrowCursor

            property real lastX: 0
            property real lastY: 0

            function focusNearestScope() {
                var item = root.parent
                while (item) {
                    if (item instanceof FocusScope) {
                        item.forceActiveFocus()
                        return
                    }
                    item = item.parent
                }
                root.forceActiveFocus()
            }

            onWheel: (wheel) => {
                if (!root.showingImage) {
                    return
                }
                const factor = wheel.angleDelta.y > 0
                    ? Theme.zoomStepFactor
                    : 1.0 / Theme.zoomStepFactor
                root.zoomAtPoint(factor, wheel.x, wheel.y)
            }

            onPressed: (mouse) => {
                focusNearestScope()
                if (!root.showingImage) {
                    mouse.accepted = false
                    return
                }
                if (mouse.button === Qt.LeftButton || mouse.button === Qt.MiddleButton) {
                    lastX = mouse.x
                    lastY = mouse.y
                }
            }

            onPositionChanged: (mouse) => {
                if (!root.showingImage || !pressed) {
                    return
                }
                if (mouse.buttons & Qt.LeftButton || mouse.buttons & Qt.MiddleButton) {
                    root.panOffsetX += mouse.x - lastX
                    root.panOffsetY += mouse.y - lastY
                    lastX = mouse.x
                    lastY = mouse.y
                    root.clampPan()
                }
            }

            onDoubleClicked: (mouse) => {
                if (!root.showingImage) {
                    return
                }
                root.resetZoom()
            }
        }

        Row {
            id: zoomButtons
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: Theme.spaceMd
            spacing: Theme.zoomButtonsSpacing
            visible: root.showingImage
            z: 2

            ToolButton {
                text: "\u2212"
                font.pixelSize: 18
                onClicked: root.zoomOut()
            }

            ToolButton {
                text: root.isZoomed ? Math.round(root.zoomLevel * 100) + "%" : "Fit"
                font.pixelSize: 12
                onClicked: {
                    if (root.isZoomed) {
                        root.resetZoom()
                    } else {
                        root.zoomToHundredPercent()
                    }
                }
            }

            ToolButton {
                text: "+"
                font.pixelSize: 18
                onClicked: root.zoomIn()
            }

            ToolButton {
                text: "\u21BA"
                font.pixelSize: 18
                visible: root.isZoomed
                onClicked: root.resetZoom()
            }
        }
    }
}
