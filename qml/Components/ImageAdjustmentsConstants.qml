pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property real bloomIntensity: 1.2
    readonly property real bloomMaximum: 1.0
    readonly property real blurWeight0: 0.1681
    readonly property real blurWeight1: 0.1542
    readonly property real blurWeight2: 0.1161
    readonly property real blurWeight3: 0.0734
    readonly property real blurWeight4: 0.04
    readonly property real blurWeight5: 0.0191
    readonly property real blurWeight6: 0.00792
    readonly property real blurWeight7: 0.00285
    readonly property real blurWeight8: 0.00088
    readonly property real blurOffsetScale1: 0.125
    readonly property real blurOffsetScale2: 0.25
    readonly property real blurOffsetScale3: 0.375
    readonly property real blurOffsetScale4: 0.5
    readonly property real blurOffsetScale5: 0.625
    readonly property real blurOffsetScale6: 0.75
    readonly property real blurOffsetScale7: 0.875
    readonly property real blurOffsetScale8: 1.0
    readonly property real blurScaleDefault: 0.25
    readonly property real blurScaleMinimum: 0.1
    readonly property real blurScaleMaximum: 1.0
}
