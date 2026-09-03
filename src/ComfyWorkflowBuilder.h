#pragma once

#include <QJsonObject>
#include <QString>

struct ComfyPilotDefaults
{
    static constexpr int canvasWidth = 512;
    static constexpr int canvasHeight = 512;
    static constexpr int refineCount = 2;
    static constexpr bool faceDetail = true;
    static constexpr bool emptyRefinePrompt = false;
    static constexpr int seed = 0;
    static constexpr int initialSteps = 8;
    static constexpr int refineSteps = 8;
    static constexpr double initialGuidance = 4.0;
    static constexpr double refineGuidance = 4.0;
    static constexpr double initialDenoise = 1.0;
    static constexpr double refineDenoise = 0.2;
    static constexpr int minCanvasSize = 64;
    static constexpr int maxCanvasSize = 8192;
    static constexpr int minRefineCount = 0;
    static constexpr int maxRefineCount = 8;
    static constexpr int minSeed = 0;
    static constexpr int maxSeed = 2147483647;
    static constexpr int minSteps = 1;
    static constexpr int maxSteps = 35;
    static constexpr double minGuidance = 0.0;
    static constexpr double maxGuidance = 10.0;
    static constexpr double minDenoise = 0.0;
    static constexpr double maxDenoise = 1.0;

    static QString serverUrl();
    static QString actionPreview();
    static QString actionNextSeed();
    static QString actionGenerate();
};

struct ComfyPilotParameters
{
    int canvasWidth = ComfyPilotDefaults::canvasWidth;
    int canvasHeight = ComfyPilotDefaults::canvasHeight;
    int refineCount = ComfyPilotDefaults::refineCount;
    bool faceDetail = ComfyPilotDefaults::faceDetail;
    bool emptyRefinePrompt = ComfyPilotDefaults::emptyRefinePrompt;
    int seed = ComfyPilotDefaults::seed;
    int initialSteps = ComfyPilotDefaults::initialSteps;
    int refineSteps = ComfyPilotDefaults::refineSteps;
    double initialGuidance = ComfyPilotDefaults::initialGuidance;
    double refineGuidance = ComfyPilotDefaults::refineGuidance;
    double initialDenoise = ComfyPilotDefaults::initialDenoise;
    double refineDenoise = ComfyPilotDefaults::refineDenoise;
    QString positivePrompt;
    QString negativePrompt;
};

class ComfyWorkflowBuilder
{
public:
    static QJsonObject buildPrompt(const ComfyPilotParameters &params,
                                   QString *error,
                                   const QString &savePrefixOverride = QString());
    static QString defaultSavePrefix();
    static QString retrySavePrefix();

private:
    struct ModelConstants
    {
        static QString checkpointName();
        static QString upscaleModelName();
        static QString samplerName();
        static QString schedulerName();
        static QString tilingStrategy();
        static QString faceMaskType();
        static QString faceMaskControl();
        static QString savePrefix();
    };
};
