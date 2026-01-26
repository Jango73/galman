import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15

import "."
import Galman 1.0

Pane {
    id: root
    property color panelBackground: Theme.panelBackground
    property alias autoEnabled: autoAdjustCheck.checked
    property alias bloomValue: bloomSlider.value
    property alias brightnessValue: brightnessSlider.value
    property alias contrastValue: contrastSlider.value
    property alias bloomRadiusValue: bloomRadiusSlider.value
    readonly property real bloomMinimum: 0.0
    readonly property real bloomMaximum: 1.0
    readonly property real bloomStep: 0.01
    readonly property real brightnessMinimum: -1.0
    readonly property real brightnessMaximum: 1.0
    readonly property real brightnessStep: 0.01
    readonly property real contrastMinimum: -1.0
    readonly property real contrastMaximum: 1.0
    readonly property real contrastStep: 0.01
    readonly property real bloomRadiusMinimum: 0.02
    readonly property real bloomRadiusMaximum: 0.25
    readonly property real bloomRadiusStep: 0.005
    readonly property real bloomDefault: 0.0
    readonly property real brightnessDefault: 0.0
    readonly property real contrastDefault: 0.0
    readonly property real bloomRadiusDefault: 0.0
    signal saveRequested()

    padding: Theme.panelPadding
    background: Rectangle {
        color: root.panelBackground
    }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spaceSm

        Label {
            text: qsTr("Adjustments")
            font.pixelSize: 14
            font.bold: true
            Layout.fillWidth: true
        }

        CheckBox {
            id: autoAdjustCheck
            text: qsTr("Auto")
            Layout.fillWidth: true
        }

        Item {
            height: Theme.spaceSm
            Layout.fillWidth: true
        }

            ColumnLayout {
                Layout.fillWidth: true
                enabled: !autoAdjustCheck.checked
                spacing: Theme.spaceSm

            Label {
                text: qsTr("Bloom")
                Layout.fillWidth: true
            }

            Slider {
                id: bloomSlider
                from: root.bloomMinimum
                to: root.bloomMaximum
                stepSize: root.bloomStep
                value: root.bloomDefault
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Bloom radius")
                Layout.fillWidth: true
            }

            Slider {
                id: bloomRadiusSlider
                from: root.bloomRadiusMinimum
                to: root.bloomRadiusMaximum
                stepSize: root.bloomRadiusStep
                value: root.bloomRadiusDefault
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Brightness")
                Layout.fillWidth: true
            }

            Slider {
                id: brightnessSlider
                from: root.brightnessMinimum
                to: root.brightnessMaximum
                stepSize: root.brightnessStep
                value: root.brightnessDefault
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Contrast")
                Layout.fillWidth: true
            }

                Slider {
                    id: contrastSlider
                    from: root.contrastMinimum
                    to: root.contrastMaximum
                    stepSize: root.contrastStep
                    value: root.contrastDefault
                    Layout.fillWidth: true
                }
            }

        Item { Layout.fillHeight: true }

        Button {
            text: qsTr("Save")
            Layout.fillWidth: true
            onClicked: root.saveRequested()
        }
    }
}
