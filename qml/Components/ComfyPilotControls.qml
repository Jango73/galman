import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Galman 1.0

import "."

Pane {
    id: root
    property var outputBrowser: null
    property color panelBackground: Theme.panelBackground
    property real fieldLabelWidth: 0

    function parseFloatField(text, fallback) {
        const normalized = String(text || "").replace(",", ".")
        const parsed = Number(normalized)
        if (!Number.isFinite(parsed)) {
            return fallback
        }
        return parsed
    }

    function floatFieldText(value) {
        if (value === undefined || value === null) {
            return ""
        }
        return String(value)
    }

    function localPathFromUrl(url) {
        const text = String(url || "")
        const prefix = "file://"
        const withoutScheme = text.indexOf(prefix) === 0 ? text.substring(prefix.length) : text
        return decodeURIComponent(withoutScheme)
    }

    padding: Theme.panelPadding
    Material.background: root.panelBackground

    ScrollView {
        id: controlScroll
        anchors.fill: parent
        clip: true
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            width: Theme.scrollBarThickness
            implicitWidth: Theme.scrollBarThickness
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
        }
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        contentWidth: controlScroll.availableWidth - Theme.scrollBarThickness
        contentHeight: controlColumn.implicitHeight

        ColumnLayout {
            id: controlColumn
            width: controlScroll.availableWidth - Theme.scrollBarThickness
            onWidthChanged: root.fieldLabelWidth = width * 0.5
            spacing: Theme.spaceSm

            Label {
                text: qsTr("Mode")
                font.pixelSize: 14
                font.bold: true
                Layout.fillWidth: true
            }

            CheckBox {
                text: qsTr("Video")
                checked: comfyPilotController ? comfyPilotController.videoEnabled : false
                enabled: !(comfyPilotController && comfyPilotController.running)
                Layout.fillWidth: true
                topPadding: Theme.controlPaddingV
                bottomPadding: Theme.controlPaddingV
                onToggled: {
                    if (comfyPilotController) {
                        comfyPilotController.videoEnabled = checked
                    }
                }
            }

            Item {
                height: Theme.sectionSpacer
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Actions")
                font.pixelSize: 14
                font.bold: true
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Seed")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                SpinBox {
                    Layout.fillWidth: true
                from: comfyPilotController.minSeed
                to: comfyPilotController.maxSeed
                stepSize: 1
                value: comfyPilotController ? comfyPilotController.seed : comfyPilotController.defaultSeed
                editable: true
                enabled: !(comfyPilotController && comfyPilotController.running)
                onValueModified: {
                    if (!visible) {
                        return
                    }
                    if (comfyPilotController) {
                        comfyPilotController.seed = value
                    }
                }
                }
            }
            RowLayout {
                id: previewButtonRow
                Layout.fillWidth: true
                spacing: Theme.spaceSm
                property real uniformButtonWidth: Math.max(previewButton.implicitWidth,
                    previewCancelButton.implicitWidth, previewNextSeedButton.implicitWidth,
                    nextSeedCancelButton.implicitWidth)

                Button {
                    id: previewButton
                    text: qsTr("Preview")
                    Layout.fillWidth: true
                    Layout.preferredWidth: previewButtonRow.uniformButtonWidth
                    visible: !(comfyPilotController && comfyPilotController.running
                        && comfyPilotController.runningAction === comfyPilotController.actionPreview)
                    enabled: comfyPilotController && !comfyPilotController.running && !comfyPilotController.videoEnabled
                    onClicked: () => {
                        if (comfyPilotController) {
                            comfyPilotController.preview()
                        }
                    }
                }

                Button {
                    id: previewCancelButton
                    text: qsTr("Cancel")
                    Layout.fillWidth: true
                    Layout.preferredWidth: previewButtonRow.uniformButtonWidth
                    Material.foreground: Theme.danger
                    visible: comfyPilotController && comfyPilotController.running
                        && comfyPilotController.runningAction === comfyPilotController.actionPreview
                    enabled: comfyPilotController && comfyPilotController.running
                    onClicked: () => {
                        if (comfyPilotController) {
                            comfyPilotController.cancel()
                        }
                    }
                }

                Button {
                    id: previewNextSeedButton
                    text: qsTr("Preview next seed")
                    Layout.fillWidth: true
                    Layout.preferredWidth: previewButtonRow.uniformButtonWidth
                    visible: !(comfyPilotController && comfyPilotController.running
                        && comfyPilotController.runningAction === comfyPilotController.actionNextSeed)
                    enabled: comfyPilotController && !comfyPilotController.running && !comfyPilotController.videoEnabled
                    onClicked: () => {
                        if (comfyPilotController) {
                            comfyPilotController.previewNextSeed()
                        }
                    }
                }

                Button {
                    id: nextSeedCancelButton
                    text: qsTr("Cancel")
                    Layout.fillWidth: true
                    Layout.preferredWidth: previewButtonRow.uniformButtonWidth
                    Material.foreground: Theme.danger
                    visible: comfyPilotController && comfyPilotController.running
                        && comfyPilotController.runningAction === comfyPilotController.actionNextSeed
                    enabled: comfyPilotController && comfyPilotController.running
                    onClicked: () => {
                        if (comfyPilotController) {
                            comfyPilotController.cancel()
                        }
                    }
                }
            }

            RowLayout {
                id: generateButtonRow
                Layout.fillWidth: true
                spacing: Theme.spaceSm
                property real uniformButtonWidth: Math.max(generateButton.implicitWidth,
                    trashPreviewNextButton.implicitWidth)

                Button {
                    id: trashPreviewNextButton
                            text: qsTr("Trash last && preview next")
                    Layout.fillWidth: true
                    Layout.preferredWidth: generateButtonRow.uniformButtonWidth
                    visible: !(comfyPilotController && comfyPilotController.running
                        && comfyPilotController.runningAction === comfyPilotController.actionGenerate)
                    enabled: comfyPilotController && !comfyPilotController.running && !comfyPilotController.videoEnabled
                            onClicked: () => {
                                if (!comfyPilotController) {
                                    return
                                }
                                const lastOutput = comfyPilotController.outputPath
                                if (lastOutput !== "" && outputBrowser.selectPath(lastOutput)) {
                                    const trashResult = outputBrowser.trashSelected()
                                    if (trashResult && trashResult.error) {
                                        console.warn(trashResult.error)
                                    }
                                }
                                comfyPilotController.previewNextSeed()
                            }
                }

                Button {
                    id: generateButton
                    text: qsTr("Generate")
                    Layout.fillWidth: true
                    Layout.preferredWidth: generateButtonRow.uniformButtonWidth
                    visible: !(comfyPilotController && comfyPilotController.running
                        && comfyPilotController.runningAction === comfyPilotController.actionGenerate)
                    enabled: comfyPilotController && !comfyPilotController.running && (!comfyPilotController.videoEnabled || (comfyPilotController.useCurrentImage && root.outputBrowser && root.outputBrowser.selectedMediaPath !== ""))
                    onClicked: () => {
                        if (!comfyPilotController) {
                            return
                        }
                        if (comfyPilotController.videoEnabled) {
                            comfyPilotController.generateVideo(root.outputBrowser ? root.outputBrowser.selectedMediaPath : "")
                        } else {
                            comfyPilotController.generate()
                        }
                    }
                }
            }

            Button {
                text: qsTr("Cancel")
                Layout.fillWidth: true
                Material.foreground: Theme.danger
                visible: comfyPilotController && comfyPilotController.running
                    && comfyPilotController.runningAction === comfyPilotController.actionGenerate
                enabled: comfyPilotController && comfyPilotController.running
                onClicked: () => {
                    if (comfyPilotController) {
                        comfyPilotController.cancel()
                    }
                }
            }

            Label {
                text: comfyPilotController ? comfyPilotController.statusMessage : ""
                visible: comfyPilotController && comfyPilotController.statusMessage !== ""
                opacity: 0.8
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Label {
                text: comfyPilotController ? comfyPilotController.errorMessage : ""
                visible: comfyPilotController && comfyPilotController.errorMessage !== ""
                color: Theme.danger
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Item {
                height: Theme.sectionSpacer
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Prompts")
                font.pixelSize: 14
                font.bold: true
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Positive prompt")
                Layout.fillWidth: true
            }

            Flickable {
                id: positivePromptFlick
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                contentWidth: positivePromptArea.width
                contentHeight: positivePromptArea.height
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: Theme.scrollBarThickness
                    implicitWidth: Theme.scrollBarThickness
                }

                TextArea {
                    id: positivePromptArea
                    width: positivePromptFlick.width
                    height: Math.max(positivePromptFlick.height, implicitHeight)
                    wrapMode: TextArea.Wrap
                    text: comfyPilotController ? comfyPilotController.positivePrompt : ""
                    enabled: !(comfyPilotController && comfyPilotController.running)
                    onTextChanged: {
                        if (comfyPilotController) {
                            comfyPilotController.positivePrompt = text
                        }
                    }
                    onCursorRectangleChanged: {
                        if (cursorRectangle.y < positivePromptFlick.contentY) {
                            positivePromptFlick.contentY = cursorRectangle.y
                        } else if (cursorRectangle.y + cursorRectangle.height
                            > positivePromptFlick.contentY + positivePromptFlick.height) {
                            positivePromptFlick.contentY = cursorRectangle.y
                                + cursorRectangle.height - positivePromptFlick.height
                        }
                    }
                }

                WheelHandler {
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onWheel: (event) => {
                        const step = event.pixelDelta.y !== 0
                            ? event.pixelDelta.y
                            : event.angleDelta.y / 4
                        const maxScroll = Math.max(0,
                            positivePromptFlick.contentHeight - positivePromptFlick.height)
                        positivePromptFlick.contentY = Math.max(0,
                            Math.min(maxScroll, positivePromptFlick.contentY - step))
                        event.accepted = true
                    }
                }
            }

            Label {
                text: qsTr("Negative prompt")
                Layout.fillWidth: true
            }

            Flickable {
                id: negativePromptFlick
                Layout.fillWidth: true
                Layout.preferredHeight: 90
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                contentWidth: negativePromptArea.width
                contentHeight: negativePromptArea.height
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: Theme.scrollBarThickness
                    implicitWidth: Theme.scrollBarThickness
                }

                TextArea {
                    id: negativePromptArea
                    width: negativePromptFlick.width
                    height: Math.max(negativePromptFlick.height, implicitHeight)
                    wrapMode: TextArea.Wrap
                    text: comfyPilotController ? comfyPilotController.negativePrompt : ""
                    enabled: !(comfyPilotController && comfyPilotController.running)
                    onTextChanged: {
                        if (comfyPilotController) {
                            comfyPilotController.negativePrompt = text
                        }
                    }
                    onCursorRectangleChanged: {
                        if (cursorRectangle.y < negativePromptFlick.contentY) {
                            negativePromptFlick.contentY = cursorRectangle.y
                        } else if (cursorRectangle.y + cursorRectangle.height
                            > negativePromptFlick.contentY + negativePromptFlick.height) {
                            negativePromptFlick.contentY = cursorRectangle.y
                                + cursorRectangle.height - negativePromptFlick.height
                        }
                    }
                }

                WheelHandler {
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onWheel: (event) => {
                        const step = event.pixelDelta.y !== 0
                            ? event.pixelDelta.y
                            : event.angleDelta.y / 4
                        const maxScroll = Math.max(0,
                            negativePromptFlick.contentHeight - negativePromptFlick.height)
                        negativePromptFlick.contentY = Math.max(0,
                            Math.min(maxScroll, negativePromptFlick.contentY - step))
                        event.accepted = true
                    }
                }
            }

            Item {
                height: Theme.sectionSpacer
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Format")
                font.pixelSize: 14
                font.bold: true
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Canvas width")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                SpinBox {
                    Layout.fillWidth: true
                from: comfyPilotController.minCanvasSize
                to: comfyPilotController.maxCanvasSize
                stepSize: 64
                value: comfyPilotController ? comfyPilotController.canvasWidth : comfyPilotController.defaultCanvasWidth
                editable: true
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                onValueModified: {
                    if (!visible) {
                        return
                    }
                    if (comfyPilotController) {
                        comfyPilotController.canvasWidth = value
                    }
                }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Canvas height")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                SpinBox {
                    Layout.fillWidth: true
                from: comfyPilotController.minCanvasSize
                to: comfyPilotController.maxCanvasSize
                stepSize: 64
                value: comfyPilotController ? comfyPilotController.canvasHeight : comfyPilotController.defaultCanvasHeight
                editable: true
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                onValueModified: {
                    if (!visible) {
                        return
                    }
                    if (comfyPilotController) {
                        comfyPilotController.canvasHeight = value
                    }
                }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm
                Label {
                    text: qsTr("Canvas size")
                    Layout.preferredWidth: root.fieldLabelWidth
                }
                SpinBox {
                    Layout.fillWidth: true
                from: comfyPilotController.minVideoCanvasSize
                to: comfyPilotController.maxVideoCanvasSize
                stepSize: 8
                value: comfyPilotController ? comfyPilotController.canvasSize : comfyPilotController.defaultCanvasSize
                editable: true
                enabled: (comfyPilotController && comfyPilotController.videoEnabled) && !(comfyPilotController && comfyPilotController.running)
                onValueModified: {
                    if (!visible) {
                        return
                    }
                    if (comfyPilotController) {
                        comfyPilotController.canvasSize = value
                    }
                }
                }
            }

            CheckBox {
                text: qsTr("Use current image")
                checked: comfyPilotController ? comfyPilotController.useCurrentImage : true
                enabled: (comfyPilotController && comfyPilotController.videoEnabled) && !(comfyPilotController && comfyPilotController.running)
                Layout.fillWidth: true
                topPadding: Theme.controlPaddingV
                bottomPadding: Theme.controlPaddingV
                onToggled: {
                    if (comfyPilotController) {
                        comfyPilotController.useCurrentImage = checked
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Duration")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                SpinBox {
                    Layout.fillWidth: true
                    from: comfyPilotController.minVideoDuration
                    to: comfyPilotController.maxVideoDuration
                    stepSize: 1
                    value: comfyPilotController ? comfyPilotController.videoDuration : comfyPilotController.defaultVideoDuration
                    editable: true
                    enabled: (comfyPilotController && comfyPilotController.videoEnabled) && !(comfyPilotController && comfyPilotController.running)
                    onValueModified: {
                        if (!visible) {
                            return
                        }
                        if (comfyPilotController) {
                            comfyPilotController.videoDuration = value
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Frame rate")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                SpinBox {
                    Layout.fillWidth: true
                    from: comfyPilotController.minVideoFrameRate
                    to: comfyPilotController.maxVideoFrameRate
                    stepSize: 1
                    value: comfyPilotController ? comfyPilotController.videoFrameRate : comfyPilotController.defaultVideoFrameRate
                    editable: true
                    enabled: (comfyPilotController && comfyPilotController.videoEnabled) && !(comfyPilotController && comfyPilotController.running)
                    onValueModified: {
                        if (!visible) {
                            return
                        }
                        if (comfyPilotController) {
                            comfyPilotController.videoFrameRate = value
                        }
                    }
                }
            }

            Item {
                height: Theme.sectionSpacer
                Layout.fillWidth: true
            }

            Label {
                text: qsTr("Passes")
                font.pixelSize: 14
                font.bold: true
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Refine pass count")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                SpinBox {
                    Layout.fillWidth: true
                from: comfyPilotController.minRefineCount
                to: comfyPilotController.maxRefineCount
                stepSize: 1
                value: comfyPilotController ? comfyPilotController.refineCount : comfyPilotController.defaultRefineCount
                editable: true
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                onValueModified: {
                    if (!visible) {
                        return
                    }
                    if (comfyPilotController) {
                        comfyPilotController.refineCount = value
                    }
                }
                }
            }

            Label {
                text: qsTr("Warning: each pass doubles width & height of image")
                color: Theme.danger
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            CheckBox {
                text: qsTr("Face detail pass")
                checked: comfyPilotController ? comfyPilotController.faceDetail : comfyPilotController.defaultFaceDetail
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                Layout.fillWidth: true
                topPadding: Theme.controlPaddingV
                bottomPadding: Theme.controlPaddingV
                onToggled: {
                    if (comfyPilotController) {
                        comfyPilotController.faceDetail = checked
                    }
                }
            }

            CheckBox {
                text: qsTr("Empty refine prompt")
                checked: comfyPilotController ? comfyPilotController.emptyRefinePrompt : false
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                Layout.fillWidth: true
                topPadding: Theme.controlPaddingV
                bottomPadding: Theme.controlPaddingV
                onToggled: {
                    if (comfyPilotController) {
                        comfyPilotController.emptyRefinePrompt = checked
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Initial steps")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                SpinBox {
                    Layout.fillWidth: true
                from: comfyPilotController.minSteps
                to: comfyPilotController.maxSteps
                stepSize: 1
                value: comfyPilotController ? comfyPilotController.initialSteps : comfyPilotController.defaultInitialSteps
                editable: true
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                onValueModified: {
                    if (!visible) {
                        return
                    }
                    if (comfyPilotController) {
                        comfyPilotController.initialSteps = value
                    }
                }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Refine steps")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                SpinBox {
                    Layout.fillWidth: true
                from: comfyPilotController.minSteps
                to: comfyPilotController.maxSteps
                stepSize: 1
                value: comfyPilotController ? comfyPilotController.refineSteps : comfyPilotController.defaultRefineSteps
                editable: true
                enabled: !(comfyPilotController && comfyPilotController.running)
                onValueModified: {
                    if (!visible) {
                        return
                    }
                    if (comfyPilotController) {
                        comfyPilotController.refineSteps = value
                    }
                }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Initial guidance")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                TextField {
                    Layout.fillWidth: true
                text: root.floatFieldText(comfyPilotController ? comfyPilotController.initialGuidance : comfyPilotController.defaultInitialGuidance)
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                onEditingFinished: {
                    if (comfyPilotController) {
                        comfyPilotController.initialGuidance = root.parseFloatField(text, comfyPilotController.initialGuidance)
                    }
                }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Refine guidance")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                TextField {
                    Layout.fillWidth: true
                text: root.floatFieldText(comfyPilotController ? comfyPilotController.refineGuidance : comfyPilotController.defaultRefineGuidance)
                enabled: !(comfyPilotController && comfyPilotController.running)
                onEditingFinished: {
                    if (comfyPilotController) {
                        comfyPilotController.refineGuidance = root.parseFloatField(text, comfyPilotController.refineGuidance)
                    }
                }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Initial denoise")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                TextField {
                    Layout.fillWidth: true
                text: root.floatFieldText(comfyPilotController ? comfyPilotController.initialDenoise : comfyPilotController.defaultInitialDenoise)
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                onEditingFinished: {
                    if (comfyPilotController) {
                        comfyPilotController.initialDenoise = root.parseFloatField(text, comfyPilotController.initialDenoise)
                    }
                }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Label {
                    text: qsTr("Refine denoise")
                    Layout.preferredWidth: root.fieldLabelWidth
                }

                TextField {
                    Layout.fillWidth: true
                text: root.floatFieldText(comfyPilotController ? comfyPilotController.refineDenoise : comfyPilotController.defaultRefineDenoise)
                enabled: !(comfyPilotController && comfyPilotController.running) && !(comfyPilotController && comfyPilotController.videoEnabled)
                onEditingFinished: {
                    if (comfyPilotController) {
                        comfyPilotController.refineDenoise = root.parseFloatField(text, comfyPilotController.refineDenoise)
                    }
                }
                }
            }

        }
    }

    DropArea {
        anchors.fill: parent
        onDropped: (drop) => {
            if (!drop || !drop.urls || drop.urls.length === 0) {
                return
            }
            if (!comfyPilotController) {
                return
            }
            for (let i = 0; i < drop.urls.length; i += 1) {
                const path = root.localPathFromUrl(drop.urls[i])
                if (path.toLowerCase().endsWith(".png")) {
                    comfyPilotController.importFromImage(path)
                    return
                }
            }
        }
    }
}
