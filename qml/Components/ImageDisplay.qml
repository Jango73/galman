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
            source: root.imagePath
            asynchronous: true
            opacity: root.useAdjustments ? root.opacityHidden : root.opacityVisible
        }

        ImageAdjustmentsEffect {
            anchors.fill: parent
            sourceItem: imageItem
            active: root.useAdjustments
            bloomValue: root.bloomValue
            bloomRadiusPercent: root.bloomRadiusPercent
            brightnessValue: root.brightnessValue
            contrastValue: root.contrastValue
        }

        Label {
            anchors.centerIn: parent
            text: root.imagePath === "" ? root.emptyText : ""
            opacity: 0.7
        }
    }
}
