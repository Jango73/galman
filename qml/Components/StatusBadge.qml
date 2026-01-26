import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 12
    height: 12

    property int status: 0
    property bool ghost: false
    property int statusPending: 1
    property int statusIdentical: 2
    property int statusDifferent: 3
    property color statusIdenticalColor: Theme.statusIdentical
    property color statusDifferentColor: Theme.statusDifferent

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        visible: (root.status === root.statusIdentical || root.status === root.statusDifferent) && !root.ghost
        color: root.status === root.statusIdentical ? root.statusIdenticalColor : root.statusDifferentColor
    }

    Item {
        id: spinner
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        visible: root.status === root.statusPending && !root.ghost

        Canvas {
            anchors.fill: parent
            onPaint: {
                const ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.lineWidth = Math.max(2, Math.round(width * 0.15))
                ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.8)
                ctx.beginPath()
                ctx.arc(width / 2, height / 2, width / 2 - ctx.lineWidth, 0, Math.PI * 1.5, false)
                ctx.stroke()
            }
        }

        RotationAnimator {
            target: spinner
            from: 0
            to: 360
            duration: 900
            loops: Animation.Infinite
            running: spinner.visible
        }
    }
}
