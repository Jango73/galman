import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15
import Galman 1.0

import "."

Pane {
    id: root
    property color panelBackground: Theme.panelBackground
    property string imagePath: ""
    property string emptyText: "No image selected"
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
        if (root.imagePath === "") {
            return ""
        }
        const separator = root.imagePath.indexOf(root.querySeparator) === root.invalidIndex
            ? root.querySeparator
            : root.queryJoiner
        return root.imagePath + separator + root.reloadQueryKey + "=" + root.reloadToken
    }
    readonly property real paintedWidth: imageItem.paintedWidth
    readonly property real paintedHeight: imageItem.paintedHeight
    readonly property real paintedHorizontalOffset: (imageItem.width - paintedWidth) * centerFactor
    readonly property real paintedVerticalOffset: (imageItem.height - paintedHeight) * centerFactor

    padding: Theme.panelPadding
    Material.background: panelBackground

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
            opacity: root.useAdjustments ? root.opacityHidden : root.opacityVisible
        }

        ImageAdjustmentsEffect {
            x: imageItem.x + root.paintedHorizontalOffset
            y: imageItem.y + root.paintedVerticalOffset
            width: root.paintedWidth
            height: root.paintedHeight
            sourceItem: imageItem
            active: root.useAdjustments
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
            text: root.imagePath === "" ? root.emptyText : ""
            opacity: 0.7
        }
    }
}
