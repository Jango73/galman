import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

Item {
    id: root
    property bool selected: false
    property bool isImage: false
    property bool isVideo: false
    property bool isDir: false
    property bool isGhost: false
    property string filePath: ""
    property string otherSidePath: ""
    property string thumbnailPath: ""
    property string thumbnailRevision: ""
    property string fileName: ""
    property string modifiedText: ""
    property bool thumbnailReady: false
    property int compareStatus: 0
    property int statusPending: 1
    property int statusIdentical: 2
    property int statusDifferent: 3
    property color statusIdenticalColor: Theme.statusIdenticalAlt
    property color statusDifferentColor: Theme.statusDifferentAlt
    property bool showRowStartIndex: false
    property string rowStartIndexText: ""
    property bool isNewer: false

    Rectangle {
        anchors.fill: parent
        radius: 0
        color: root.selected
            ? Qt.rgba(Material.accent.r, Material.accent.g, Material.accent.b, 0.18)
            : "transparent"
        border.color: root.selected ? Material.accent : "transparent"
        border.width: root.selected ? 2 : 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceMd
        spacing: Theme.spaceMd

        Item {
            id: previewContainer
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.fillHeight: true
            readonly property real squareSize: Math.min(width, height)
            readonly property string fallbackImagePath: root.isGhost && root.otherSidePath !== ""
                ? root.otherSidePath
                : root.filePath
            readonly property string effectiveThumbnailPath: root.thumbnailPath !== ""
                ? root.thumbnailPath
                : (root.isImage ? fallbackImagePath : "")
            readonly property bool hasPreviewSource: effectiveThumbnailPath !== ""
            readonly property string previewSource: hasPreviewSource
                ? ("file://" + effectiveThumbnailPath + (root.thumbnailRevision !== "" ? ("?v=" + encodeURIComponent(root.thumbnailRevision)) : ""))
                : ""

            Image {
                width: previewContainer.squareSize
                height: previewContainer.squareSize
                anchors.centerIn: parent
                source: previewContainer.previewSource
                sourceSize.width: Math.max(1, previewContainer.squareSize * 2)
                sourceSize.height: Math.max(1, previewContainer.squareSize * 2)
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                opacity: root.isGhost ? Theme.ghostPreviewOpacity : 1.0
                visible: previewContainer.hasPreviewSource && root.thumbnailReady
                onStatusChanged: root.thumbnailReady = (status === Image.Ready)
                onSourceChanged: root.thumbnailReady = false
            }

            Canvas {
                anchors.centerIn: parent
                width: Math.max(1, previewContainer.squareSize * 0.78)
                height: Math.max(1, previewContainer.squareSize * 0.78)
                visible: !previewContainer.hasPreviewSource || !root.thumbnailReady
                onVisibleChanged: requestPaint()
                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                Component.onCompleted: requestPaint()

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.clearRect(0, 0, width, height)
                    const baseAlpha = root.isGhost ? 0.35 : 0.85
                    const base = Qt.rgba(
                        Material.foreground.r,
                        Material.foreground.g,
                        Material.foreground.b,
                        baseAlpha
                    )
                    ctx.fillStyle = base

                    if (root.isDir) {
                        ctx.beginPath()
                        ctx.moveTo(4, height * 0.35)
                        ctx.lineTo(width * 0.42, height * 0.35)
                        ctx.lineTo(width * 0.52, height * 0.2)
                        ctx.lineTo(width - 4, height * 0.2)
                        ctx.lineTo(width - 4, height - 4)
                        ctx.lineTo(4, height - 4)
                        ctx.closePath()
                        ctx.fill()
                    } else {
                        ctx.beginPath()
                        ctx.moveTo(6, 4)
                        ctx.lineTo(width * 0.64, 4)
                        ctx.lineTo(width - 4, height * 0.34)
                        ctx.lineTo(width - 4, height - 4)
                        ctx.lineTo(6, height - 4)
                        ctx.closePath()
                        ctx.fill()

                        ctx.fillStyle = Qt.rgba(
                            Material.foreground.r,
                            Material.foreground.g,
                            Material.foreground.b,
                            root.isGhost ? 0.25 : 0.55
                        )
                        ctx.beginPath()
                        ctx.moveTo(width * 0.64, 4)
                        ctx.lineTo(width - 4, height * 0.34)
                        ctx.lineTo(width * 0.64, height * 0.34)
                        ctx.closePath()
                        ctx.fill()
                    }

                    if (root.isGhost) {
                        ctx.strokeStyle = Qt.rgba(
                            Material.foreground.r,
                            Material.foreground.g,
                            Material.foreground.b,
                            0.5
                        )
                        ctx.lineWidth = 5
                        ctx.lineCap = "round"
                        ctx.beginPath()
                        ctx.moveTo(width * 0.08, height * 0.95)
                        ctx.lineTo(width * 0.92, height * 0.05)
                        ctx.stroke()
                    }
                }
            }
        }

        Label {
            id: nameLabel
            text: Theme.elideFileName(root.fileName)
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            Layout.minimumHeight: implicitHeight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }

        Label {
            id: modifiedLabel
            text: root.modifiedText
            visible: root.modifiedText !== ""
            opacity: 0.7
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            Layout.minimumHeight: implicitHeight
        }
    }

    Canvas {
        id: newerBadge
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: Theme.spaceSm
        anchors.leftMargin: Theme.spaceSm + 12 + Theme.spaceXs
        width: 12
        height: 12
        visible: root.isNewer && !root.isGhost
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

    StatusBadge {
        id: statusBadge
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: Theme.spaceSm
        status: root.compareStatus
        ghost: root.isGhost
        statusPending: root.statusPending
        statusIdentical: root.statusIdentical
        statusDifferent: root.statusDifferent
        statusIdenticalColor: root.statusIdenticalColor
        statusDifferentColor: root.statusDifferentColor
    }

    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: Theme.rowIndexBadgeMargin
        visible: root.showRowStartIndex && root.rowStartIndexText !== ""
        color: Theme.rowIndexBadgeBackground
        radius: Theme.rowIndexBadgeRadius
        implicitWidth: rowStartIndexLabel.implicitWidth + (Theme.rowIndexBadgePaddingHorizontal * 2)
        implicitHeight: rowStartIndexLabel.implicitHeight + (Theme.rowIndexBadgePaddingVertical * 2)

        Label {
            id: rowStartIndexLabel
            anchors.centerIn: parent
            text: root.rowStartIndexText
            color: Theme.rowIndexBadgeTextColor
            font.pixelSize: Theme.rowIndexBadgeFontPixelSize
        }
    }
}
