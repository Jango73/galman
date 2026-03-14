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
    property bool useAdjustments: false
    property real bloomValue: 0.0
    property real bloomRadiusPercent: 0.12
    property real brightnessValue: 0.0
    property real contrastValue: 0.0
    readonly property real opacityVisible: 1.0
    readonly property real opacityHidden: 0.0
    readonly property real centerFactor: 0.5
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

    padding: Theme.panelPadding
    Material.background: panelBackground

    onMediaPathChanged: syncVideoPlayback()
    onMediaIsVideoChanged: syncVideoPlayback()
    onVisibleChanged: syncVideoPlayback()
    onVideoPlaybackActiveChanged: syncVideoPlayback()

    function syncVideoPlayback() {
        if (root.videoPlaybackActive) {
            mediaPlayer.play()
            return
        }
        mediaPlayer.stop()
    }

    Item {
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
        }

        Image {
            id: imageItem
            anchors.fill: parent
            fillMode: Image.PreserveAspectFit
            cache: false
            source: root.resolvedImageSource
            asynchronous: true
            visible: root.showingImage
            opacity: root.useAdjustments ? root.opacityHidden : root.opacityVisible
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

        ImageAdjustmentsEffect {
            x: imageItem.x + root.paintedHorizontalOffset
            y: imageItem.y + root.paintedVerticalOffset
            width: root.paintedWidth
            height: root.paintedHeight
            sourceItem: imageItem
            active: root.useAdjustments && root.showingImage
            bloomValue: root.bloomValue
            bloomRadiusPercent: root.bloomRadiusPercent
            brightnessValue: root.brightnessValue
            contrastValue: root.contrastValue
            sourceRect: Qt.rect(root.paintedHorizontalOffset,
                                root.paintedVerticalOffset,
                                root.paintedWidth,
                                root.paintedHeight)
        }

        Label {
            anchors.centerIn: parent
            text: root.mediaPath === "" ? root.emptyText : ""
            opacity: 0.7
        }
    }
}
