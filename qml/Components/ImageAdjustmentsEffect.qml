import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls.Material 2.15

import "."

Item {
    id: root
    property Item sourceItem: null
    property bool active: false
    property real bloomValue: 0.0
    property real bloomRadiusPercent: 0.12
    property real brightnessValue: 0.0
    property real contrastValue: 0.0
    readonly property real opacityHidden: 0.0
    readonly property real contrastBase: 1.0
    readonly property real contrastPivot: 0.5
    readonly property real bloomIntensity: ImageAdjustmentsConstants.bloomIntensity
    readonly property real bloomMaximum: ImageAdjustmentsConstants.bloomMaximum
    readonly property real blurWeight0: ImageAdjustmentsConstants.blurWeight0
    readonly property real blurWeight1: ImageAdjustmentsConstants.blurWeight1
    readonly property real blurWeight2: ImageAdjustmentsConstants.blurWeight2
    readonly property real blurWeight3: ImageAdjustmentsConstants.blurWeight3
    readonly property real blurWeight4: ImageAdjustmentsConstants.blurWeight4
    readonly property real blurWeight5: ImageAdjustmentsConstants.blurWeight5
    readonly property real blurWeight6: ImageAdjustmentsConstants.blurWeight6
    readonly property real blurWeight7: ImageAdjustmentsConstants.blurWeight7
    readonly property real blurWeight8: ImageAdjustmentsConstants.blurWeight8
    readonly property real blurOffsetScale1: ImageAdjustmentsConstants.blurOffsetScale1
    readonly property real blurOffsetScale2: ImageAdjustmentsConstants.blurOffsetScale2
    readonly property real blurOffsetScale3: ImageAdjustmentsConstants.blurOffsetScale3
    readonly property real blurOffsetScale4: ImageAdjustmentsConstants.blurOffsetScale4
    readonly property real blurOffsetScale5: ImageAdjustmentsConstants.blurOffsetScale5
    readonly property real blurOffsetScale6: ImageAdjustmentsConstants.blurOffsetScale6
    readonly property real blurOffsetScale7: ImageAdjustmentsConstants.blurOffsetScale7
    readonly property real blurOffsetScale8: ImageAdjustmentsConstants.blurOffsetScale8
    readonly property real blurScale: ImageAdjustmentsConstants.blurScaleDefault
    readonly property real blurScaleMinimum: ImageAdjustmentsConstants.blurScaleMinimum
    readonly property real blurScaleMaximum: ImageAdjustmentsConstants.blurScaleMaximum
    readonly property real one: 1.0
    readonly property int minPixelSize: 1
    readonly property real blurRadiusPixels: Math.max(Math.max(width, height), 1) * bloomRadiusPercent
    readonly property real blurSourceWidth: Math.max(minPixelSize, Math.round(width * blurScale))
    readonly property real blurSourceHeight: Math.max(minPixelSize, Math.round(height * blurScale))
    readonly property real blurRadiusScaled: blurRadiusPixels * blurScale
    readonly property real blurOffset1: Math.max(1.0, blurRadiusScaled * blurOffsetScale1)
    readonly property real blurOffset2: Math.max(1.0, blurRadiusScaled * blurOffsetScale2)
    readonly property real blurOffset3: Math.max(1.0, blurRadiusScaled * blurOffsetScale3)
    readonly property real blurOffset4: Math.max(1.0, blurRadiusScaled * blurOffsetScale4)
    readonly property real blurOffset5: Math.max(1.0, blurRadiusScaled * blurOffsetScale5)
    readonly property real blurOffset6: Math.max(1.0, blurRadiusScaled * blurOffsetScale6)
    readonly property real blurOffset7: Math.max(1.0, blurRadiusScaled * blurOffsetScale7)
    readonly property real blurOffset8: Math.max(1.0, blurRadiusScaled * blurOffsetScale8)
    property rect sourceRect: Qt.rect(opacityHidden,
                                      opacityHidden,
                                      sourceItem ? sourceItem.width : opacityHidden,
                                      sourceItem ? sourceItem.height : opacityHidden)

    visible: active && sourceItem

    ShaderEffectSource {
        id: imageSource
        sourceItem: root.sourceItem
        hideSource: root.active
        live: true
        visible: root.active && root.sourceItem
        sourceRect: root.sourceRect
    }

    ShaderEffect {
        id: bloomExtract
        width: root.blurSourceWidth
        height: root.blurSourceHeight
        visible: root.active && root.sourceItem
        property variant source: imageSource
        fragmentShader: "qrc:/shaders/qml/Shaders/ImageBloomPass.frag.qsb"
    }

    ShaderEffectSource {
        id: bloomExtractSource
        sourceItem: bloomExtract
        hideSource: true
        live: true
        visible: root.active && root.sourceItem
    }

    ShaderEffect {
        id: bloomBlurHorizontal
        width: root.blurSourceWidth
        height: root.blurSourceHeight
        visible: root.active && root.sourceItem
        property variant source: bloomExtractSource
        property vector2d direction: Qt.vector2d(root.one, root.opacityHidden)
        property vector2d texelSize: Qt.vector2d(
            root.one / Math.max(width, root.minPixelSize),
            root.one / Math.max(height, root.minPixelSize)
        )
        property real weight0: root.blurWeight0
        property real weight1: root.blurWeight1
        property real weight2: root.blurWeight2
        property real weight3: root.blurWeight3
        property real weight4: root.blurWeight4
        property real weight5: root.blurWeight5
        property real weight6: root.blurWeight6
        property real weight7: root.blurWeight7
        property real weight8: root.blurWeight8
        property real offset1: root.blurOffset1
        property real offset2: root.blurOffset2
        property real offset3: root.blurOffset3
        property real offset4: root.blurOffset4
        property real offset5: root.blurOffset5
        property real offset6: root.blurOffset6
        property real offset7: root.blurOffset7
        property real offset8: root.blurOffset8
        fragmentShader: "qrc:/shaders/qml/Shaders/ImageBloomBlur.frag.qsb"
    }

    ShaderEffectSource {
        id: bloomBlurHorizontalSource
        sourceItem: bloomBlurHorizontal
        hideSource: true
        live: true
        visible: root.active && root.sourceItem
    }

    ShaderEffect {
        id: bloomBlurVertical
        width: root.blurSourceWidth
        height: root.blurSourceHeight
        visible: root.active && root.sourceItem
        property variant source: bloomBlurHorizontalSource
        property vector2d direction: Qt.vector2d(root.opacityHidden, root.one)
        property vector2d texelSize: Qt.vector2d(
            root.one / Math.max(width, root.minPixelSize),
            root.one / Math.max(height, root.minPixelSize)
        )
        property real weight0: root.blurWeight0
        property real weight1: root.blurWeight1
        property real weight2: root.blurWeight2
        property real weight3: root.blurWeight3
        property real weight4: root.blurWeight4
        property real weight5: root.blurWeight5
        property real weight6: root.blurWeight6
        property real weight7: root.blurWeight7
        property real weight8: root.blurWeight8
        property real offset1: root.blurOffset1
        property real offset2: root.blurOffset2
        property real offset3: root.blurOffset3
        property real offset4: root.blurOffset4
        property real offset5: root.blurOffset5
        property real offset6: root.blurOffset6
        property real offset7: root.blurOffset7
        property real offset8: root.blurOffset8
        fragmentShader: "qrc:/shaders/qml/Shaders/ImageBloomBlur.frag.qsb"
    }

    ShaderEffectSource {
        id: bloomBlurSource
        sourceItem: bloomBlurVertical
        hideSource: true
        live: true
        visible: root.active && root.sourceItem
    }

    ShaderEffect {
        id: imageComposite
        anchors.fill: parent
        visible: root.active && root.sourceItem
        property variant source: imageSource
        property variant bloomSource: bloomBlurSource
        property real bloom: root.bloomValue
        property real brightness: root.brightnessValue
        property real contrast: root.contrastValue
        property real pivot: root.contrastPivot
        property real bloomScale: root.bloomIntensity
        property real contrastBase: root.contrastBase
        fragmentShader: "qrc:/shaders/qml/Shaders/ImageBloomComposite.frag.qsb"
    }
}
