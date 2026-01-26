import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "."
import Galman 1.0

FocusRememberingScope {
    id: root
    property var leftBrowser: null
    property var rightBrowser: null
    property color panelBackground: Theme.panelBackground
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
            if (leftBrowser) {
                leftBrowser.selectAdjacentImage(-1)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Right) {
            if (leftBrowser) {
                leftBrowser.selectAdjacentImage(1)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Home) {
            if (leftBrowser) {
                leftBrowser.selectBoundaryImage(true)
            }
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_End) {
            if (leftBrowser) {
                leftBrowser.selectBoundaryImage(false)
            }
            event.accepted = true
            return
        }
        event.accepted = false
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: Theme.spaceLg

        ImageDisplay {
            Layout.fillWidth: true
            Layout.fillHeight: true
            panelBackground: root.panelBackground
            imagePath: leftBrowser && leftBrowser.selectedImagePath !== ""
                ? ("file://" + leftBrowser.selectedImagePath)
                : ""
            compareStatus: leftBrowser ? leftBrowser.selectedCompareStatus : 0
            ghost: leftBrowser ? leftBrowser.selectedGhost : false
            statusPending: leftBrowser ? leftBrowser.statusPending : 1
            statusIdentical: leftBrowser ? leftBrowser.statusIdentical : 2
            statusDifferent: leftBrowser ? leftBrowser.statusDifferent : 3
            statusIdenticalColor: leftBrowser ? leftBrowser.statusIdenticalColor : Theme.statusIdentical
            statusDifferentColor: leftBrowser ? leftBrowser.statusDifferentColor : Theme.statusDifferent
        }

        ImageDisplay {
            Layout.fillWidth: true
            Layout.fillHeight: true
            panelBackground: root.panelBackground
            imagePath: rightBrowser && rightBrowser.selectedImagePath !== ""
                ? ("file://" + rightBrowser.selectedImagePath)
                : ""
            compareStatus: rightBrowser ? rightBrowser.selectedCompareStatus : 0
            ghost: rightBrowser ? rightBrowser.selectedGhost : false
            statusPending: rightBrowser ? rightBrowser.statusPending : 1
            statusIdentical: rightBrowser ? rightBrowser.statusIdentical : 2
            statusDifferent: rightBrowser ? rightBrowser.statusDifferent : 3
            statusIdenticalColor: rightBrowser ? rightBrowser.statusIdenticalColor : Theme.statusIdentical
            statusDifferentColor: rightBrowser ? rightBrowser.statusDifferentColor : Theme.statusDifferent
        }
    }
}
