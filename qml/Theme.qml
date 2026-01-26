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
    readonly property int spaceXs: 4
    readonly property int spaceSm: 6
    readonly property int spaceMd: 8
    readonly property int spaceLg: 12
    readonly property int controlPaddingV: spaceXs
    readonly property int sectionSpacer: spaceMd
    readonly property int panelPadding: spaceLg
    readonly property int windowMargin: spaceLg
    readonly property int statusBarPadding: spaceSm
}
