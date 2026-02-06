import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

Item {
    id: root
    property bool selected: false
    property bool isImage: false
    property bool isDir: false
    property bool isGhost: false
    property string filePath: ""
    property string otherSidePath: ""
    property string fileName: ""
    property string modifiedText: ""
    property bool thumbnailReady: false
    property int compareStatus: 0
    property int statusPending: 1
    property int statusIdentical: 2
    property int statusDifferent: 3
    property color statusIdenticalColor: Theme.statusIdenticalAlt
    property color statusDifferentColor: Theme.statusDifferentAlt

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
            readonly property string previewPath: root.isGhost && root.otherSidePath !== ""
                ? root.otherSidePath
                : root.filePath
            readonly property bool hasPreviewSource: root.isImage && previewPath !== ""

            Image {
                width: previewContainer.squareSize
                height: previewContainer.squareSize
                anchors.centerIn: parent
                source: previewContainer.hasPreviewSource ? ("file://" + previewContainer.previewPath) : ""
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

    StatusBadge {
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
}
