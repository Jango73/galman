import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

Item {
    id: root
    property var selectedPaths: []
    property string selectedPath: ""
    property string selectedName: ""
    property bool selectedIsImage: false
    property int selectedFileCount: 0
    property real selectedTotalBytes: 0
    property int imageWidth: 0
    property int imageHeight: 0
    property bool imageReady: false
    property bool syncEnabled: false
    property bool syncPreviewEnabled: false
    property bool hideIdentical: false
    property Item previousPanelFocusItem: null
    property Item nextPanelFocusItem: null
    readonly property Item firstFocusItem: syncModeCheckBox
    readonly property Item lastFocusItem: runScriptButton
    signal errorRaised(string message)

    onSyncEnabledChanged: {
        if (!syncEnabled) {
            syncPreviewEnabled = false
        }
    }
    property string scriptPath: ""
    property string scriptName: ""
    property string scriptDescription: ""
    property var scriptControls: []
    property var scriptValues: ({})
    property string scriptResult: ""
    property color panelBackground: Material.background
    readonly property bool multiSelection: selectedPaths && selectedPaths.length > 1

    function sanitizeTextValue(value, fallback) {
        if (value === null || value === undefined) {
            return fallback
        }
        const stringValue = String(value)
        if (stringValue.trim().toLowerCase() === "nan") {
            return fallback
        }
        return stringValue
    }

    function scriptKeyFromPath(path) {
        const normalized = String(path || "").replace(/\\/g, "/")
        const lastSlash = normalized.lastIndexOf("/")
        const filename = lastSlash >= 0 ? normalized.slice(lastSlash + 1) : normalized
        const dot = filename.lastIndexOf(".")
        return dot > 0 ? filename.slice(0, dot) : filename
    }

    function globalKey(scriptPath, controlId) {
        const key = scriptKeyFromPath(scriptPath)
        if (!key) {
            return controlId
        }
        return key + "." + controlId
    }

    function buildDefaultValues() {
        const nextValues = {}
        for (let i = 0; i < scriptControls.length; i += 1) {
            const control = scriptControls[i]
            if (!control || !control.id) {
                continue
            }
            if (control.hasOwnProperty("default")) {
                let value = control.default
                if (control.type === "text" || control.type === "path" || control.type === "multiline") {
                    value = sanitizeTextValue(value, "")
                }
                nextValues[control.id] = value
                continue
            }
            if (control.type === "checkbox") {
                nextValues[control.id] = false
            } else if (control.type === "number" || control.type === "slider") {
                nextValues[control.id] = control.min !== undefined ? control.min : 0
            } else if (control.type === "combo" && control.options && control.options.length > 0) {
                nextValues[control.id] = control.options[0]
            } else {
                nextValues[control.id] = ""
            }
        }
        return nextValues
    }

    function rebuildScriptValues() {
        if (!scriptPath) {
            scriptValues = buildDefaultValues()
            scriptResult = ""
            return
        }
        const defaults = buildDefaultValues()
        const storedLocal = scriptEngine.loadScriptParams(scriptPath)
        const storedGlobal = scriptEngine.loadScriptParams("__global__")
        const nextValues = Object.assign({}, defaults, storedLocal)
        for (let i = 0; i < scriptControls.length; i += 1) {
            const control = scriptControls[i]
            if (!control || !control.id) {
                continue
            }
            if (control.scope === "global") {
                const key = globalKey(scriptPath, control.id)
                if (storedGlobal.hasOwnProperty(key)) {
                    nextValues[control.id] = storedGlobal[key]
                }
            }
            if (control.type === "text" || control.type === "path" || control.type === "multiline") {
                const fallback = defaults.hasOwnProperty(control.id) ? defaults[control.id] : ""
                nextValues[control.id] = sanitizeTextValue(nextValues[control.id], fallback)
            }
        }
        scriptValues = nextValues
        scriptResult = ""
    }

    onSelectedPathsChanged: {
        if (selectedPaths && selectedPaths.length === 1) {
            selectedPath = selectedPaths[0]
            const parts = selectedPath.split("/")
            selectedName = parts.length > 0 ? parts[parts.length - 1] : selectedPath
        } else {
            selectedPath = ""
            selectedName = ""
            selectedIsImage = false
        }
    }

    onScriptControlsChanged: rebuildScriptValues()
    onScriptPathChanged: rebuildScriptValues()

    Pane {
        anchors.fill: parent
        padding: Theme.panelPadding
        Material.background: panelBackground

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            ScrollView {
                id: controlScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 1
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                contentWidth: controlColumn.implicitWidth
                contentHeight: controlColumn.implicitHeight

                ColumnLayout {
                    id: controlColumn
                    width: controlScroll.availableWidth
                    spacing: Theme.spaceSm

                    Label {
                        text: qsTr("General")
                        font.pixelSize: 14
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    CheckBox {
                        id: syncModeCheckBox
                        text: qsTr("Synchronize mode")
                        checked: root.syncEnabled
                        topPadding: Theme.controlPaddingV
                        bottomPadding: Theme.controlPaddingV
                        Layout.fillWidth: true
                        KeyNavigation.backtab: root.previousPanelFocusItem
                        onToggled: root.syncEnabled = checked
                    }

                    CheckBox {
                        text: qsTr("Hide identical")
                        checked: root.hideIdentical
                        enabled: root.syncEnabled
                        topPadding: Theme.controlPaddingV
                        bottomPadding: Theme.controlPaddingV
                        Layout.fillWidth: true
                        onToggled: root.hideIdentical = checked
                    }

                    CheckBox {
                        text: qsTr("Show sync preview")
                        checked: root.syncPreviewEnabled
                        enabled: root.syncEnabled
                        topPadding: Theme.controlPaddingV
                        bottomPadding: Theme.controlPaddingV
                        Layout.fillWidth: true
                        onToggled: root.syncPreviewEnabled = checked
                    }

                    Item {
                        height: Theme.sectionSpacer
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("Script")
                        font.pixelSize: 14
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Label {
                        text: scriptName === "" ? qsTr("No script selected") : scriptName
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        wrapMode: Text.NoWrap
                        opacity: scriptName === "" ? 0.7 : 1.0
                        Layout.fillWidth: true
                    }

                    Label {
                        visible: scriptDescription !== ""
                        text: scriptDescription
                        wrapMode: Text.WordWrap
                        opacity: 0.8
                        Layout.fillWidth: true
                    }

                    ColumnLayout {
                        Layout.fillWidth: true

                        Repeater {
                            model: scriptControls

                            delegate: Item {
                                width: parent ? parent.width : 0
                                height: fieldColumn.implicitHeight

                                ColumnLayout {
                                    id: fieldColumn
                                    anchors.fill: parent
                                    spacing: Theme.spaceXs

                                    Label {
                                        visible: modelData.type !== "checkbox"
                                        text: modelData.label || modelData.id || ""
                                        Layout.fillWidth: true
                                    }

                                    TextField {
                                        visible: modelData.type === "text" || modelData.type === "path"
                                        Layout.fillWidth: true
                                        placeholderText: modelData.placeholder || ""
                                        text: root.scriptValues[modelData.id] !== undefined
                                            ? root.sanitizeTextValue(root.scriptValues[modelData.id], "")
                                            : ""
                                        onTextChanged: {
                                            root.scriptValues[modelData.id] = text
                                        }
                                    }

                                    TextArea {
                                        visible: modelData.type === "multiline"
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 90
                                        placeholderText: modelData.placeholder || ""
                                        text: root.scriptValues[modelData.id] !== undefined
                                            ? root.sanitizeTextValue(root.scriptValues[modelData.id], "")
                                            : ""
                                        wrapMode: TextArea.Wrap
                                        onTextChanged: {
                                            root.scriptValues[modelData.id] = text
                                        }
                                    }

                                    CheckBox {
                                        visible: modelData.type === "checkbox"
                                        text: modelData.label || modelData.id || ""
                                        checked: !!root.scriptValues[modelData.id]
                                        Layout.fillWidth: true
                                        onToggled: root.scriptValues[modelData.id] = checked
                                    }

                                    ComboBox {
                                        visible: modelData.type === "combo"
                                        Layout.fillWidth: true
                                        model: modelData.options || []
                                        onCurrentIndexChanged: {
                                            if (currentIndex >= 0 && model && model.length > 0) {
                                                root.scriptValues[modelData.id] = model[currentIndex]
                                            }
                                        }
                                        Component.onCompleted: {
                                            const value = root.scriptValues[modelData.id]
                                            if (value !== undefined && model && model.length > 0) {
                                                const idx = model.indexOf(value)
                                                currentIndex = idx >= 0 ? idx : 0
                                            }
                                        }
                                    }

                                    SpinBox {
                                        visible: modelData.type === "number"
                                        Layout.fillWidth: true
                                        from: modelData.min !== undefined ? modelData.min : 0
                                        to: modelData.max !== undefined ? modelData.max : 9999
                                        stepSize: modelData.step !== undefined ? modelData.step : 1
                                        value: root.scriptValues[modelData.id] !== undefined
                                            ? Number(root.scriptValues[modelData.id])
                                            : (modelData.min !== undefined ? modelData.min : 0)
                                        editable: true
                                        onValueModified: {
                                            if (!visible) {
                                                return
                                            }
                                            root.scriptValues[modelData.id] = value
                                        }
                                    }

                                    Slider {
                                        visible: modelData.type === "slider"
                                        Layout.fillWidth: true
                                        from: modelData.min !== undefined ? modelData.min : 0
                                        to: modelData.max !== undefined ? modelData.max : 100
                                        stepSize: modelData.step !== undefined ? modelData.step : 1
                                        value: root.scriptValues[modelData.id] !== undefined
                                            ? Number(root.scriptValues[modelData.id])
                                            : (modelData.min !== undefined ? modelData.min : 0)
                                        onValueChanged: {
                                            if (!visible) {
                                                return
                                            }
                                            root.scriptValues[modelData.id] = value
                                        }
                                    }
                                }
                            }
                        }

                        Button {
                            id: runScriptButton
                            text: qsTr("Run script")
                            Layout.fillWidth: true
                            enabled: scriptPath !== "" && !root.syncEnabled
                            KeyNavigation.tab: root.nextPanelFocusItem
                            onClicked: {
                                if (scriptPath === "") {
                                    return
                                }
                                const localParams = {}
                                const globalParams = {}
                                for (let i = 0; i < scriptControls.length; i += 1) {
                                    const control = scriptControls[i]
                                    if (!control || !control.id) {
                                        continue
                                    }
                                const value = root.scriptValues[control.id]
                                if (control.scope === "global") {
                                    globalParams[globalKey(scriptPath, control.id)] = value
                                } else {
                                    localParams[control.id] = value
                                }
                                }
                                scriptEngine.saveScriptParams(scriptPath, localParams)
                                if (Object.keys(globalParams).length > 0) {
                                    scriptEngine.saveScriptParams("__global__", globalParams)
                                }
                                const params = Object.assign({}, root.scriptValues, { _scriptPath: scriptPath })
                                const output = scriptEngine.runScript(scriptPath, params)
                                if (Array.isArray(output)) {
                                    const okCount = output.filter(entry => entry && entry.ok).length
                                    if (output.length === 0) {
                                        scriptResult = qsTr("No images selected")
                                    } else if (okCount === output.length) {
                                        scriptResult = qsTr("Done: %1 / %2").arg(okCount).arg(output.length)
                                    } else {
                                        let lastError = ""
                                        for (let i = output.length - 1; i >= 0; i -= 1) {
                                            const entry = output[i]
                                            if (entry && entry.error) {
                                                lastError = entry.error
                                                break
                                            }
                                        }
                                        scriptResult = qsTr("Done: %1 / %2").arg(okCount).arg(output.length)
                                        if (lastError) {
                                            scriptResult += qsTr(" (error: %1)").arg(lastError)
                                            root.errorRaised(lastError)
                                        }
                                    }
                                } else if (output && output.error) {
                                    scriptResult = String(output.error)
                                    root.errorRaised(String(output.error))
                                } else {
                                    scriptResult = String(output)
                                }
                            }
                        }

                        Label {
                            text: scriptResult
                            visible: scriptResult !== ""
                            opacity: 0.7
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: false
                Layout.preferredHeight: root.height / 3

                Column {
                    anchors.fill: parent
                    spacing: Theme.spaceXs

                    Label {
                        text: qsTr("Selection")
                        font.pixelSize: 14
                        font.bold: true
                    }

                    Label {
                        text: selectedPath === ""
                            ? qsTr("No item selected")
                            : qsTr("Name: %1").arg(Theme.elideFileName(selectedName))
                        visible: !root.multiSelection
                        elide: Text.ElideRight
                        maximumLineCount: 2
                        wrapMode: Text.WordWrap
                        opacity: selectedPath === "" ? 0.7 : 1.0
                    }

                    Label {
                        text: qsTr("Files: %1").arg(selectedFileCount)
                        visible: root.multiSelection
                        opacity: 0.9
                    }

                    Label {
                        text: qsTr("Total size: %1").arg(Theme.formatByteSize(selectedTotalBytes))
                        visible: root.multiSelection
                        opacity: 0.8
                    }

                    Label {
                        text: qsTr("Width: %1").arg(imageReady ? imageWidth : qsTr("-"))
                        visible: !root.multiSelection
                        opacity: selectedPath === "" ? 0.7 : 0.8
                    }

                    Label {
                        text: qsTr("Height: %1").arg(imageReady ? imageHeight : qsTr("-"))
                        visible: !root.multiSelection
                        opacity: 0.8
                    }
                }
            }
        }
    }

    Image {
        id: imageProbe
        source: selectedIsImage && selectedPath !== "" ? ("file://" + selectedPath) : ""
        asynchronous: true
        visible: false

        onStatusChanged: {
            if (status === Image.Ready) {
                imageWidth = Math.round(imageProbe.implicitWidth)
                imageHeight = Math.round(imageProbe.implicitHeight)
                imageReady = true
            } else {
                imageReady = false
                imageWidth = 0
                imageHeight = 0
            }
        }
    }
}
