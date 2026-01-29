pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property color panelBackground: "#20242a"
    readonly property color statusBarBackground: "#1b1f24"
    readonly property color statusIdentical: "#6bbf59"
    readonly property color statusDifferent: "#d6b14a"
    readonly property color statusMissing: "#c06565"
    readonly property color statusIdenticalAlt: "#86e1aa"
    readonly property color statusDifferentAlt: "#ffcf5a"
    readonly property color transparentColor: "transparent"
    readonly property color modalOverlayColor: Qt.rgba(0, 0, 0, 0.4)
    readonly property int spaceXs: 4
    readonly property int spaceSm: 6
    readonly property int spaceMd: 8
    readonly property int spaceLg: 12
    readonly property int controlPaddingV: spaceXs
    readonly property int sectionSpacer: spaceMd
    readonly property int panelPadding: spaceLg
    readonly property int windowMargin: spaceLg
    readonly property int statusBarPadding: spaceSm
    readonly property int focusFrameWidth: 2
    readonly property int focusFrameRadius: 0
    readonly property int fileNameMaxLength: 20
    readonly property string fileNameEllipsis: "..."

    function elideFileName(name) {
        const raw = String(name || "")
        if (raw.length <= fileNameMaxLength) {
            return raw
        }
        const keepLength = Math.max(0, fileNameMaxLength - fileNameEllipsis.length)
        return raw.slice(0, keepLength) + fileNameEllipsis
    }
}
