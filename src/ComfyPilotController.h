#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QVariantMap>

#include "ComfyPilotWorker.h"

class ComfyPilotController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString positivePrompt READ positivePrompt WRITE setPositivePrompt NOTIFY positivePromptChanged)
    Q_PROPERTY(QString negativePrompt READ negativePrompt WRITE setNegativePrompt NOTIFY negativePromptChanged)
    Q_PROPERTY(int canvasWidth READ canvasWidth WRITE setCanvasWidth NOTIFY canvasWidthChanged)
    Q_PROPERTY(int canvasHeight READ canvasHeight WRITE setCanvasHeight NOTIFY canvasHeightChanged)
    Q_PROPERTY(int refineCount READ refineCount WRITE setRefineCount NOTIFY refineCountChanged)
    Q_PROPERTY(bool faceDetail READ faceDetail WRITE setFaceDetail NOTIFY faceDetailChanged)
    Q_PROPERTY(bool emptyRefinePrompt READ emptyRefinePrompt WRITE setEmptyRefinePrompt NOTIFY emptyRefinePromptChanged)
    Q_PROPERTY(int seed READ seed WRITE setSeed NOTIFY seedChanged)
    Q_PROPERTY(int initialSteps READ initialSteps WRITE setInitialSteps NOTIFY initialStepsChanged)
    Q_PROPERTY(int refineSteps READ refineSteps WRITE setRefineSteps NOTIFY refineStepsChanged)
    Q_PROPERTY(double initialGuidance READ initialGuidance WRITE setInitialGuidance NOTIFY initialGuidanceChanged)
    Q_PROPERTY(double refineGuidance READ refineGuidance WRITE setRefineGuidance NOTIFY refineGuidanceChanged)
    Q_PROPERTY(double initialDenoise READ initialDenoise WRITE setInitialDenoise NOTIFY initialDenoiseChanged)
    Q_PROPERTY(double refineDenoise READ refineDenoise WRITE setRefineDenoise NOTIFY refineDenoiseChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString runningAction READ runningAction NOTIFY runningChanged)
    Q_PROPERTY(QString defaultServerUrl READ defaultServerUrl CONSTANT)
    Q_PROPERTY(int defaultCanvasWidth READ defaultCanvasWidth CONSTANT)
    Q_PROPERTY(int defaultCanvasHeight READ defaultCanvasHeight CONSTANT)
    Q_PROPERTY(int defaultRefineCount READ defaultRefineCount CONSTANT)
    Q_PROPERTY(bool defaultFaceDetail READ defaultFaceDetail CONSTANT)
    Q_PROPERTY(int defaultSeed READ defaultSeed CONSTANT)
    Q_PROPERTY(int defaultInitialSteps READ defaultInitialSteps CONSTANT)
    Q_PROPERTY(int defaultRefineSteps READ defaultRefineSteps CONSTANT)
    Q_PROPERTY(double defaultInitialGuidance READ defaultInitialGuidance CONSTANT)
    Q_PROPERTY(double defaultRefineGuidance READ defaultRefineGuidance CONSTANT)
    Q_PROPERTY(double defaultInitialDenoise READ defaultInitialDenoise CONSTANT)
    Q_PROPERTY(double defaultRefineDenoise READ defaultRefineDenoise CONSTANT)
    Q_PROPERTY(int minCanvasSize READ minCanvasSize CONSTANT)
    Q_PROPERTY(int maxCanvasSize READ maxCanvasSize CONSTANT)
    Q_PROPERTY(int minRefineCount READ minRefineCount CONSTANT)
    Q_PROPERTY(int maxRefineCount READ maxRefineCount CONSTANT)
    Q_PROPERTY(int minSeed READ minSeed CONSTANT)
    Q_PROPERTY(int maxSeed READ maxSeed CONSTANT)
    Q_PROPERTY(int minSteps READ minSteps CONSTANT)
    Q_PROPERTY(int maxSteps READ maxSteps CONSTANT)
    Q_PROPERTY(double minGuidance READ minGuidance CONSTANT)
    Q_PROPERTY(double maxGuidance READ maxGuidance CONSTANT)
    Q_PROPERTY(double minDenoise READ minDenoise CONSTANT)
    Q_PROPERTY(double maxDenoise READ maxDenoise CONSTANT)
    Q_PROPERTY(QString actionPreview READ actionPreview CONSTANT)
    Q_PROPERTY(QString actionNextSeed READ actionNextSeed CONSTANT)
    Q_PROPERTY(QString actionGenerate READ actionGenerate CONSTANT)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged)
    Q_PROPERTY(bool outputIsVideo READ outputIsVideo NOTIFY outputPathChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit ComfyPilotController(QObject *parent = nullptr);
    ~ComfyPilotController() override;

    QString serverUrl() const;
    QString positivePrompt() const;
    QString negativePrompt() const;
    int canvasWidth() const;
    int canvasHeight() const;
    int refineCount() const;
    bool faceDetail() const;
    bool emptyRefinePrompt() const;
    int seed() const;
    int initialSteps() const;
    int refineSteps() const;
    double initialGuidance() const;
    double refineGuidance() const;
    double initialDenoise() const;
    double refineDenoise() const;
    bool running() const;
    QString runningAction() const;
    QString defaultServerUrl() const;
    int defaultCanvasWidth() const;
    int defaultCanvasHeight() const;
    int defaultRefineCount() const;
    bool defaultFaceDetail() const;
    int defaultSeed() const;
    int defaultInitialSteps() const;
    int defaultRefineSteps() const;
    double defaultInitialGuidance() const;
    double defaultRefineGuidance() const;
    double defaultInitialDenoise() const;
    double defaultRefineDenoise() const;
    int minCanvasSize() const;
    int maxCanvasSize() const;
    int minRefineCount() const;
    int maxRefineCount() const;
    int minSeed() const;
    int maxSeed() const;
    int minSteps() const;
    int maxSteps() const;
    double minGuidance() const;
    double maxGuidance() const;
    double minDenoise() const;
    double maxDenoise() const;
    QString actionPreview() const;
    QString actionNextSeed() const;
    QString actionGenerate() const;
    QString outputPath() const;
    bool outputIsVideo() const;
    QString errorMessage() const;
    QString statusMessage() const;

    void setServerUrl(const QString &value);
    void setPositivePrompt(const QString &value);
    void setNegativePrompt(const QString &value);
    void setCanvasWidth(int value);
    void setCanvasHeight(int value);
    void setRefineCount(int value);
    void setFaceDetail(bool value);
    void setEmptyRefinePrompt(bool value);
    void setSeed(int value);
    void setInitialSteps(int value);
    void setRefineSteps(int value);
    void setInitialGuidance(double value);
    void setRefineGuidance(double value);
    void setInitialDenoise(double value);
    void setRefineDenoise(double value);

    Q_INVOKABLE void generate();
    Q_INVOKABLE void preview();
    Q_INVOKABLE void previewNextSeed();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE QVariantMap importFromImage(const QString &imagePath);

signals:
    void serverUrlChanged();
    void positivePromptChanged();
    void negativePromptChanged();
    void canvasWidthChanged();
    void canvasHeightChanged();
    void refineCountChanged();
    void faceDetailChanged();
    void emptyRefinePromptChanged();
    void seedChanged();
    void initialStepsChanged();
    void refineStepsChanged();
    void initialGuidanceChanged();
    void refineGuidanceChanged();
    void initialDenoiseChanged();
    void refineDenoiseChanged();
    void runningChanged();
    void outputPathChanged();
    void errorMessageChanged();
    void statusMessageChanged();

private slots:
    void handleWorkerFinished(QVariantMap result);

private:
    ComfyPilotJob buildJob() const;
    void launchJob(const ComfyPilotJob &job);
    void loadParameters();
    void saveParameters() const;
    void setRunning(bool value);
    void setOutputPath(const QString &value, bool isVideo);
    void setErrorMessage(const QString &value);
    void setStatusMessage(const QString &value);
    void cleanupThread();

    QString m_serverUrl = ComfyPilotDefaults::serverUrl();
    QString m_positivePrompt;
    QString m_negativePrompt;
    int m_canvasWidth = ComfyPilotDefaults::canvasWidth;
    int m_canvasHeight = ComfyPilotDefaults::canvasHeight;
    int m_refineCount = ComfyPilotDefaults::refineCount;
    bool m_faceDetail = ComfyPilotDefaults::faceDetail;
    bool m_emptyRefinePrompt = ComfyPilotDefaults::emptyRefinePrompt;
    int m_seed = ComfyPilotDefaults::seed;
    int m_initialSteps = ComfyPilotDefaults::initialSteps;
    int m_refineSteps = ComfyPilotDefaults::refineSteps;
    double m_initialGuidance = ComfyPilotDefaults::initialGuidance;
    double m_refineGuidance = ComfyPilotDefaults::refineGuidance;
    double m_initialDenoise = ComfyPilotDefaults::initialDenoise;
    double m_refineDenoise = ComfyPilotDefaults::refineDenoise;
    bool m_running = false;
    QString m_runningAction;
    QString m_outputPath;
    bool m_outputIsVideo = false;
    QString m_errorMessage;
    QString m_statusMessage;
    QThread *m_thread = nullptr;
    ComfyPilotWorker *m_worker = nullptr;
};
