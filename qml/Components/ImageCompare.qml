import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "."
import Galman 1.0

FocusRememberingScope {
    id: root
    property var leftBrowser: null
    property var rightBrowser: null
    property var navigationBrowser: null
    property color panelBackground: Theme.panelBackground
    property int reloadToken: 0
    property int reloadTokenIncrement: 1
    signal copyLeftRequested()
    signal copyRightRequested()
    signal closeRequested()
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
            const target = navigationBrowser ? navigationBrowser : leftBrowser
            if (target) {
                target.selectAdjacentImage(-1)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Right) {
            const target = navigationBrowser ? navigationBrowser : leftBrowser
            if (target) {
                target.selectAdjacentImage(1)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Home) {
            const target = navigationBrowser ? navigationBrowser : leftBrowser
            if (target) {
                target.selectBoundaryImage(true)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_End) {
            const target = navigationBrowser ? navigationBrowser : leftBrowser
            if (target) {
                target.selectBoundaryImage(false)
            }
            event.accepted = true
            return
        }
        event.accepted = false
    }

    Item {
        id: imagePane
        anchors.fill: parent

        FocusFrame {
            active: root.activeFocus
            z: 2
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 0
            spacing: Theme.spaceLg

            ImageDisplay {
                id: leftImageDisplay
                Layout.fillWidth: true
                Layout.fillHeight: true
                panelBackground: root.panelBackground
                imagePath: leftBrowser && leftBrowser.selectedImagePath !== ""
                    ? ("file://" + leftBrowser.selectedImagePath)
                    : ""
                reloadToken: root.reloadToken
                compareStatus: leftBrowser ? leftBrowser.selectedCompareStatus : 0
                ghost: leftBrowser ? leftBrowser.selectedGhost : false
                statusPending: leftBrowser ? leftBrowser.statusPending : 1
                statusIdentical: leftBrowser ? leftBrowser.statusIdentical : 2
                statusDifferent: leftBrowser ? leftBrowser.statusDifferent : 3
                statusIdenticalColor: leftBrowser ? leftBrowser.statusIdenticalColor : Theme.statusIdentical
                statusDifferentColor: leftBrowser ? leftBrowser.statusDifferentColor : Theme.statusDifferent
            }

            ImageDisplay {
                id: rightImageDisplay
                Layout.fillWidth: true
                Layout.fillHeight: true
                panelBackground: root.panelBackground
                imagePath: rightBrowser && rightBrowser.selectedImagePath !== ""
                    ? ("file://" + rightBrowser.selectedImagePath)
                    : ""
                reloadToken: root.reloadToken
                compareStatus: rightBrowser ? rightBrowser.selectedCompareStatus : 0
                ghost: rightBrowser ? rightBrowser.selectedGhost : false
                statusPending: rightBrowser ? rightBrowser.statusPending : 1
                statusIdentical: rightBrowser ? rightBrowser.statusIdentical : 2
                statusDifferent: rightBrowser ? rightBrowser.statusDifferent : 3
                statusIdenticalColor: rightBrowser ? rightBrowser.statusIdenticalColor : Theme.statusIdentical
                statusDifferentColor: rightBrowser ? rightBrowser.statusDifferentColor : Theme.statusDifferent
            }
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onPressed: (mouse) => {
                root.forceActiveFocus()
                mouse.accepted = false
            }
        }
    }

    function refreshImages() {
        reloadToken += reloadTokenIncrement
    }
}
