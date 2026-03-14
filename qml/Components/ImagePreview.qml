import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

import "."
import Galman 1.0

FocusRememberingScope {
    id: root
    property var browser: null
    property color panelBackground: Theme.panelBackground
    property bool adjustmentsVisible: false
    property int reloadToken: 0
    property int reloadTokenIncrement: 1
    readonly property bool adjustmentsAvailable: browser ? browser.selectedIsImage : false
    readonly property bool adjustmentsActive: root.adjustmentsVisible && root.adjustmentsAvailable
    signal copyLeftRequested()
    signal copyRightRequested()
    signal closeRequested()
    signal saveSucceeded(string message)
    clip: true

    onBrowserChanged: {
        if (!root.adjustmentsAvailable) {
            root.adjustmentsVisible = false
        }
    }

    Connections {
        target: browser
        function onSelectedIsImageChanged() {
            if (!root.adjustmentsAvailable) {
                root.adjustmentsVisible = false
            }
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
        if (event.key === Qt.Key_Escape) {
            root.closeRequested()
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Left) {
            if (browser) {
                browser.selectAdjacentMedia(-1)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Right) {
            if (browser) {
                browser.selectAdjacentMedia(1)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Home) {
            if (browser) {
                browser.selectBoundaryMedia(true)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_End) {
            if (browser) {
                browser.selectBoundaryMedia(false)
            }
            event.accepted = true
            return
        }
        event.accepted = false
    }

    Item {
        anchors.fill: parent

        RowLayout {
            anchors.fill: parent
            spacing: Theme.spaceMd

            FocusScope {
                id: imageArea
                Layout.fillWidth: true
                Layout.fillHeight: true

                FocusFrame {
                    active: imageArea.activeFocus
                    z: 4
                }

                MediaDisplay {
                    id: mainMediaDisplay
                    anchors.fill: parent
                    panelBackground: root.panelBackground
                    mediaPath: browser && browser.selectedMediaPath !== ""
                        ? ("file://" + browser.selectedMediaPath)
                        : ""
                    mediaIsVideo: browser ? browser.selectedIsVideo : false
                    reloadToken: root.reloadToken
                    compareStatus: browser ? browser.selectedCompareStatus : 0
                    ghost: browser ? browser.selectedGhost : false
                    statusPending: browser ? browser.statusPending : 1
                    statusIdentical: browser ? browser.statusIdentical : 2
                    statusDifferent: browser ? browser.statusDifferent : 3
                    statusIdenticalColor: browser ? browser.statusIdenticalColor : Theme.statusIdentical
                    statusDifferentColor: browser ? browser.statusDifferentColor : Theme.statusDifferent
                    useAdjustments: root.adjustmentsActive
                    bloomValue: adjustmentsPanel.bloomValue
                    bloomRadiusPercent: adjustmentsPanel.bloomRadiusValue
                    brightnessValue: adjustmentsPanel.brightnessValue
                    contrastValue: adjustmentsPanel.contrastValue
                }

                ToolButton {
                    id: adjustmentsToggle
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: Theme.spaceSm
                    text: qsTr("Adjustments")
                    visible: root.adjustmentsAvailable
                    checkable: true
                    checked: root.adjustmentsVisible
                    onToggled: root.adjustmentsVisible = checked && root.adjustmentsAvailable
                }

                Rectangle {
                    id: pathBar
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: pathLabel.implicitHeight + Theme.spaceSm * 2
                    color: Qt.rgba(0, 0, 0, 0.45)
                    visible: pathLabel.text !== ""
                    z: 2
                }

                Label {
                    id: pathLabel
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: Theme.spaceSm
                    text: browser ? browser.selectedMediaPath : ""
                    color: "white"
                    wrapMode: Text.WrapAnywhere
                    elide: Text.ElideNone
                    z: 3
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onPressed: (mouse) => {
                        imageArea.forceActiveFocus()
                        mouse.accepted = false
                    }
                }
            }

            ImageAdjustmentsPanel {
                id: adjustmentsPanel
                visible: root.adjustmentsActive
                Layout.preferredWidth: root.adjustmentsActive ? 260 : 0
                Layout.minimumWidth: root.adjustmentsActive ? 220 : 0
                Layout.fillHeight: root.adjustmentsActive
                panelBackground: root.panelBackground
                onSaveRequested: root.saveAdjustedImage()
            }
        }

        ImageAutoAdjuster {
            id: autoAdjuster
            imagePath: browser && browser.selectedIsImage ? browser.selectedMediaPath : ""
            autoEnabled: adjustmentsPanel.autoEnabled
        }

        Binding {
            target: adjustmentsPanel
            property: "brightnessValue"
            value: autoAdjuster.brightnessValue
            when: adjustmentsPanel.autoEnabled
        }

        Binding {
            target: adjustmentsPanel
            property: "contrastValue"
            value: autoAdjuster.contrastValue
            when: adjustmentsPanel.autoEnabled
        }

        Item {
            id: offscreenHost
            visible: root.adjustmentsAvailable
            opacity: 0.0
            width: offscreenImage.status === Image.Ready ? offscreenImage.implicitWidth : 1
            height: offscreenImage.status === Image.Ready ? offscreenImage.implicitHeight : 1
            anchors.left: parent.left
            anchors.top: parent.top
            z: -1

            Image {
                id: offscreenImage
                anchors.fill: parent
                source: mainMediaDisplay.resolvedImageSource
                cache: false
                asynchronous: false
                fillMode: Image.PreserveAspectFit
            }

            ImageAdjustmentsEffect {
                anchors.fill: parent
                sourceItem: offscreenImage
                active: root.adjustmentsActive
                bloomValue: adjustmentsPanel.bloomValue
                bloomRadiusPercent: adjustmentsPanel.bloomRadiusValue
                brightnessValue: adjustmentsPanel.brightnessValue
                contrastValue: adjustmentsPanel.contrastValue
            }
        }
    }

    function saveAdjustedImage() {
        if (!browser || !browser.selectedIsImage || !browser.selectedMediaPath) {
            return
        }
        if (offscreenImage.status !== Image.Ready) {
            return
        }
        offscreenHost.grabToImage(function(result) {
            if (!result || !result.image) {
                return
            }
            const saveResult = scriptEngine.saveAdjustedImage(result.image, browser.selectedMediaPath)
            if (saveResult && saveResult.error) {
                console.warn(saveResult.error)
                return
            }
            root.saveSucceeded(qsTr("Saved"))
        })
    }

    function refreshImages() {
        reloadToken += reloadTokenIncrement
    }
}
