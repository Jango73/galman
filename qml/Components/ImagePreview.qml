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
    signal copyLeftRequested()
    signal copyRightRequested()
    signal closeRequested()
    signal saveSucceeded(string message)
    clip: true

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
                browser.selectAdjacentImage(-1)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Right) {
            if (browser) {
                browser.selectAdjacentImage(1)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Home) {
            if (browser) {
                browser.selectBoundaryImage(true)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_End) {
            if (browser) {
                browser.selectBoundaryImage(false)
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

            Item {
                id: imageArea
                Layout.fillWidth: true
                Layout.fillHeight: true

                ImageDisplay {
                    anchors.fill: parent
                    panelBackground: root.panelBackground
                    imagePath: browser && browser.selectedImagePath !== ""
                        ? ("file://" + browser.selectedImagePath)
                        : ""
                    compareStatus: browser ? browser.selectedCompareStatus : 0
                    ghost: browser ? browser.selectedGhost : false
                    statusPending: browser ? browser.statusPending : 1
                    statusIdentical: browser ? browser.statusIdentical : 2
                    statusDifferent: browser ? browser.statusDifferent : 3
                    statusIdenticalColor: browser ? browser.statusIdenticalColor : Theme.statusIdentical
                    statusDifferentColor: browser ? browser.statusDifferentColor : Theme.statusDifferent
                    useAdjustments: root.adjustmentsVisible
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
                    checkable: true
                    checked: root.adjustmentsVisible
                    onToggled: root.adjustmentsVisible = checked
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
                    text: browser ? browser.selectedImagePath : ""
                    color: "white"
                    wrapMode: Text.WrapAnywhere
                    elide: Text.ElideNone
                    z: 3
                }
            }

            ImageAdjustmentsPanel {
                id: adjustmentsPanel
                visible: root.adjustmentsVisible
                Layout.preferredWidth: root.adjustmentsVisible ? 260 : 0
                Layout.minimumWidth: root.adjustmentsVisible ? 220 : 0
                Layout.fillHeight: root.adjustmentsVisible
                panelBackground: root.panelBackground
                onSaveRequested: root.saveAdjustedImage()
            }
        }

        Item {
            id: offscreenHost
            visible: true
            opacity: 0.0
            width: offscreenImage.status === Image.Ready ? offscreenImage.implicitWidth : 1
            height: offscreenImage.status === Image.Ready ? offscreenImage.implicitHeight : 1
            anchors.left: parent.left
            anchors.top: parent.top
            z: -1

            Image {
                id: offscreenImage
                anchors.fill: parent
                source: browser && browser.selectedImagePath !== ""
                    ? ("file://" + browser.selectedImagePath)
                    : ""
                cache: false
                asynchronous: false
                fillMode: Image.PreserveAspectFit
            }

            ImageAdjustmentsEffect {
                anchors.fill: parent
                sourceItem: offscreenImage
                active: true
                bloomValue: adjustmentsPanel.bloomValue
                bloomRadiusPercent: adjustmentsPanel.bloomRadiusValue
                brightnessValue: adjustmentsPanel.brightnessValue
                contrastValue: adjustmentsPanel.contrastValue
            }
        }
    }

    function saveAdjustedImage() {
        if (!browser || !browser.selectedImagePath) {
            return
        }
        if (offscreenImage.status !== Image.Ready) {
            return
        }
        offscreenHost.grabToImage(function(result) {
            if (!result || !result.image) {
                return
            }
            const saveResult = scriptEngine.saveAdjustedImage(result.image, browser.selectedImagePath)
            if (saveResult && saveResult.error) {
                console.warn(saveResult.error)
                return
            }
            root.saveSucceeded(qsTr("Saved"))
        })
    }
}
