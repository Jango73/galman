import QtQuick 2.15
import QtQuick.Controls.Material 2.15
import Galman 1.0

Rectangle {
    id: root
    property bool active: false

    anchors.fill: parent
    color: Theme.transparentColor
    radius: Theme.focusFrameRadius
    border.color: root.active ? Material.accent : Theme.transparentColor
    border.width: root.active ? Theme.focusFrameWidth : 0
}
