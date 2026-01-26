import QtQuick 2.15
import QtQuick.Window 2.15

FocusScope {
    id: root
    property bool rememberFocus: true
    property Item rememberedFocusItem: null

    onVisibleChanged: {
        if (!rememberFocus) {
            return
        }
        if (visible) {
            rememberedFocusItem = Window.activeFocusItem
            forceActiveFocus()
            return
        }
        const target = rememberedFocusItem
        rememberedFocusItem = null
        Qt.callLater(() => {
            if (target && target.visible) {
                target.forceActiveFocus()
            }
        })
    }
}
