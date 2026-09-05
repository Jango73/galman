/************************************************************************\

    Galman - Picture gallery manager
    Copyright (C) 2026 Jango73

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

\************************************************************************/

#include "ComfyPilotController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QSet>

#include "ApplicationSettings.h"
#include "ComfyClient.h"
#include "ComfyPilotWorker.h"
#include "ComfyPromptImporter.h"
#include "PlatformUtils.h"
#include "VideoThumbnailUtils.h"

namespace
{

const char settingsGroup[] = "comfyPilot";
const char settingsOutputPath[] = "outputPath";

bool isMediaOutputInfo(const QFileInfo &info)
{
    if (!info.exists() || !info.isFile()) {
        return false;
    }
    if (VideoThumbnailUtils::isVideoFile(info)) {
        return true;
    }
    static const QSet<QString> imageFormats = []() {
        QSet<QString> formats;
        const QList<QByteArray> supported = QImageReader::supportedImageFormats();
        for (const QByteArray &format : supported) {
            formats.insert(QString::fromLatin1(format).toLower());
        }
        return formats;
    }();
    return imageFormats.contains(info.suffix().toLower());
}

QString findLatestMediaOutput(const QString &folderPath)
{
    const QDir folder(folderPath);
    if (!folder.exists()) {
        return QString();
    }
    const QFileInfoList entries = folder.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    QString latestPath;
    qint64 latestModified = -1;
    for (const QFileInfo &entry : entries) {
        if (!isMediaOutputInfo(entry)) {
            continue;
        }
        const qint64 modified = entry.lastModified().toMSecsSinceEpoch();
        if (modified > latestModified) {
            latestModified = modified;
            latestPath = entry.absoluteFilePath();
        }
    }
    return latestPath;
}

} // namespace

ComfyPilotController::ComfyPilotController(QObject *parent)
    : QObject(parent)
{
    loadParameters();
}

ComfyPilotController::~ComfyPilotController()
{
    cancel();
    cleanupThread();
}

QString ComfyPilotController::serverUrl() const
{
    return m_serverUrl;
}

QString ComfyPilotController::positivePrompt() const
{
    return m_positivePrompt;
}

QString ComfyPilotController::negativePrompt() const
{
    return m_negativePrompt;
}

int ComfyPilotController::canvasWidth() const
{
    return m_canvasWidth;
}

int ComfyPilotController::canvasHeight() const
{
    return m_canvasHeight;
}

int ComfyPilotController::refineCount() const
{
    return m_refineCount;
}

bool ComfyPilotController::faceDetail() const
{
    return m_faceDetail;
}

bool ComfyPilotController::emptyRefinePrompt() const
{
    return m_emptyRefinePrompt;
}

int ComfyPilotController::seed() const
{
    return m_seed;
}

int ComfyPilotController::initialSteps() const
{
    return m_initialSteps;
}

int ComfyPilotController::refineSteps() const
{
    return m_refineSteps;
}

double ComfyPilotController::initialGuidance() const
{
    return m_initialGuidance;
}

double ComfyPilotController::refineGuidance() const
{
    return m_refineGuidance;
}

double ComfyPilotController::initialDenoise() const
{
    return m_initialDenoise;
}

double ComfyPilotController::refineDenoise() const
{
    return m_refineDenoise;
}

bool ComfyPilotController::videoEnabled() const
{
    return m_videoEnabled;
}

bool ComfyPilotController::useCurrentImage() const
{
    return m_useCurrentImage;
}

int ComfyPilotController::videoDuration() const
{
    return m_videoDuration;
}

int ComfyPilotController::videoFrameRate() const
{
    return m_videoFrameRate;
}

int ComfyPilotController::canvasSize() const
{
    return m_canvasSize;
}

bool ComfyPilotController::running() const
{
    return m_running;
}

QString ComfyPilotController::runningAction() const
{
    return m_runningAction;
}

QString ComfyPilotController::defaultServerUrl() const
{
    return ComfyPilotDefaults::serverUrl();
}

int ComfyPilotController::defaultCanvasWidth() const
{
    return ComfyPilotDefaults::canvasWidth;
}

int ComfyPilotController::defaultCanvasHeight() const
{
    return ComfyPilotDefaults::canvasHeight;
}

int ComfyPilotController::defaultRefineCount() const
{
    return ComfyPilotDefaults::refineCount;
}

bool ComfyPilotController::defaultFaceDetail() const
{
    return ComfyPilotDefaults::faceDetail;
}

int ComfyPilotController::defaultSeed() const
{
    return ComfyPilotDefaults::seed;
}

int ComfyPilotController::defaultInitialSteps() const
{
    return ComfyPilotDefaults::initialSteps;
}

int ComfyPilotController::defaultRefineSteps() const
{
    return ComfyPilotDefaults::refineSteps;
}

double ComfyPilotController::defaultInitialGuidance() const
{
    return ComfyPilotDefaults::initialGuidance;
}

double ComfyPilotController::defaultRefineGuidance() const
{
    return ComfyPilotDefaults::refineGuidance;
}

double ComfyPilotController::defaultInitialDenoise() const
{
    return ComfyPilotDefaults::initialDenoise;
}

double ComfyPilotController::defaultRefineDenoise() const
{
    return ComfyPilotDefaults::refineDenoise;
}

bool ComfyPilotController::defaultVideoEnabled() const
{
    return ComfyPilotDefaults::videoEnabled;
}

bool ComfyPilotController::defaultUseCurrentImage() const
{
    return ComfyPilotDefaults::useCurrentImage;
}

int ComfyPilotController::defaultVideoDuration() const
{
    return ComfyPilotDefaults::videoDuration;
}

int ComfyPilotController::defaultVideoFrameRate() const
{
    return ComfyPilotDefaults::videoFrameRate;
}

int ComfyPilotController::defaultCanvasSize() const
{
    return ComfyPilotDefaults::canvasSize;
}

int ComfyPilotController::minCanvasSize() const
{
    return ComfyPilotDefaults::minCanvasSize;
}

int ComfyPilotController::maxCanvasSize() const
{
    return ComfyPilotDefaults::maxCanvasSize;
}

int ComfyPilotController::minRefineCount() const
{
    return ComfyPilotDefaults::minRefineCount;
}

int ComfyPilotController::maxRefineCount() const
{
    return ComfyPilotDefaults::maxRefineCount;
}

int ComfyPilotController::minSeed() const
{
    return ComfyPilotDefaults::minSeed;
}

int ComfyPilotController::maxSeed() const
{
    return ComfyPilotDefaults::maxSeed;
}

int ComfyPilotController::minSteps() const
{
    return ComfyPilotDefaults::minSteps;
}

int ComfyPilotController::maxSteps() const
{
    return ComfyPilotDefaults::maxSteps;
}

double ComfyPilotController::minGuidance() const
{
    return ComfyPilotDefaults::minGuidance;
}

double ComfyPilotController::maxGuidance() const
{
    return ComfyPilotDefaults::maxGuidance;
}

double ComfyPilotController::minDenoise() const
{
    return ComfyPilotDefaults::minDenoise;
}

double ComfyPilotController::maxDenoise() const
{
    return ComfyPilotDefaults::maxDenoise;
}

int ComfyPilotController::minVideoDuration() const
{
    return ComfyPilotDefaults::minVideoDuration;
}

int ComfyPilotController::maxVideoDuration() const
{
    return ComfyPilotDefaults::maxVideoDuration;
}

int ComfyPilotController::minVideoFrameRate() const
{
    return ComfyPilotDefaults::minVideoFrameRate;
}

int ComfyPilotController::maxVideoFrameRate() const
{
    return ComfyPilotDefaults::maxVideoFrameRate;
}

int ComfyPilotController::minVideoCanvasSize() const
{
    return ComfyPilotDefaults::minVideoCanvasSize;
}

int ComfyPilotController::maxVideoCanvasSize() const
{
    return ComfyPilotDefaults::maxVideoCanvasSize;
}

QString ComfyPilotController::actionPreview() const
{
    return ComfyPilotDefaults::actionPreview();
}

QString ComfyPilotController::actionNextSeed() const
{
    return ComfyPilotDefaults::actionNextSeed();
}

QString ComfyPilotController::actionGenerate() const
{
    return ComfyPilotDefaults::actionGenerate();
}

QString ComfyPilotController::outputPath() const
{
    return m_outputPath;
}

bool ComfyPilotController::outputIsVideo() const
{
    return m_outputIsVideo;
}

QString ComfyPilotController::errorMessage() const
{
    return m_errorMessage;
}

QString ComfyPilotController::statusMessage() const
{
    return m_statusMessage;
}

void ComfyPilotController::setServerUrl(const QString &value)
{
    if (m_serverUrl == value) {
        return;
    }
    m_serverUrl = value;
    emit serverUrlChanged();
    saveParameters();
}

void ComfyPilotController::setPositivePrompt(const QString &value)
{
    if (m_positivePrompt == value) {
        return;
    }
    m_positivePrompt = value;
    emit positivePromptChanged();
    saveParameters();
}

void ComfyPilotController::setNegativePrompt(const QString &value)
{
    if (m_negativePrompt == value) {
        return;
    }
    m_negativePrompt = value;
    emit negativePromptChanged();
    saveParameters();
}

void ComfyPilotController::setCanvasWidth(int value)
{
    if (m_canvasWidth == value) {
        return;
    }
    m_canvasWidth = value;
    emit canvasWidthChanged();
    saveParameters();
}

void ComfyPilotController::setCanvasHeight(int value)
{
    if (m_canvasHeight == value) {
        return;
    }
    m_canvasHeight = value;
    emit canvasHeightChanged();
    saveParameters();
}

void ComfyPilotController::setRefineCount(int value)
{
    if (m_refineCount == value) {
        return;
    }
    m_refineCount = value;
    emit refineCountChanged();
    saveParameters();
}

void ComfyPilotController::setFaceDetail(bool value)
{
    if (m_faceDetail == value) {
        return;
    }
    m_faceDetail = value;
    emit faceDetailChanged();
    saveParameters();
}

void ComfyPilotController::setEmptyRefinePrompt(bool value)
{
    if (m_emptyRefinePrompt == value) {
        return;
    }
    m_emptyRefinePrompt = value;
    emit emptyRefinePromptChanged();
    saveParameters();
}

void ComfyPilotController::setSeed(int value)
{
    if (m_seed == value) {
        return;
    }
    m_seed = value;
    emit seedChanged();
    saveParameters();
}

void ComfyPilotController::setInitialSteps(int value)
{
    if (m_initialSteps == value) {
        return;
    }
    m_initialSteps = value;
    emit initialStepsChanged();
    saveParameters();
}

void ComfyPilotController::setRefineSteps(int value)
{
    if (m_refineSteps == value) {
        return;
    }
    m_refineSteps = value;
    emit refineStepsChanged();
    saveParameters();
}

void ComfyPilotController::setInitialGuidance(double value)
{
    if (qFuzzyCompare(m_initialGuidance + 1.0, value + 1.0)) {
        return;
    }
    m_initialGuidance = value;
    emit initialGuidanceChanged();
    saveParameters();
}

void ComfyPilotController::setRefineGuidance(double value)
{
    if (qFuzzyCompare(m_refineGuidance + 1.0, value + 1.0)) {
        return;
    }
    m_refineGuidance = value;
    emit refineGuidanceChanged();
    saveParameters();
}

void ComfyPilotController::setInitialDenoise(double value)
{
    if (qFuzzyCompare(m_initialDenoise + 1.0, value + 1.0)) {
        return;
    }
    m_initialDenoise = value;
    emit initialDenoiseChanged();
    saveParameters();
}

void ComfyPilotController::setRefineDenoise(double value)
{
    if (qFuzzyCompare(m_refineDenoise + 1.0, value + 1.0)) {
        return;
    }
    m_refineDenoise = value;
    emit refineDenoiseChanged();
    saveParameters();
}

void ComfyPilotController::setVideoEnabled(bool value)
{
    if (m_videoEnabled == value) {
        return;
    }
    m_videoEnabled = value;
    emit videoEnabledChanged();
    saveParameters();
}

void ComfyPilotController::setUseCurrentImage(bool value)
{
    if (m_useCurrentImage == value) {
        return;
    }
    m_useCurrentImage = value;
    emit useCurrentImageChanged();
    saveParameters();
}

void ComfyPilotController::setVideoDuration(int value)
{
    if (m_videoDuration == value) {
        return;
    }
    m_videoDuration = value;
    emit videoDurationChanged();
    saveParameters();
}

void ComfyPilotController::setVideoFrameRate(int value)
{
    if (m_videoFrameRate == value) {
        return;
    }
    m_videoFrameRate = value;
    emit videoFrameRateChanged();
    saveParameters();
}

void ComfyPilotController::setCanvasSize(int value)
{
    if (m_canvasSize == value) {
        return;
    }
    m_canvasSize = value;
    emit canvasSizeChanged();
    saveParameters();
}

/**
 * @brief Starts an asynchronous ComfyUI generation in a worker thread.
 */
void ComfyPilotController::generate()
{
    if (m_running) {
        qWarning() << "ComfyPilot generate rejected: already running";
        return;
    }
    if (m_videoEnabled) {
        qWarning() << "ComfyPilot generate rejected: video mode needs an input image";
        setErrorMessage(tr("Enable \"Use current image\" and select an image to generate video"));
        return;
    }
    qInfo() << "ComfyPilot generate requested:"
            << "canvas=" << m_canvasWidth << "x" << m_canvasHeight
            << "refines=" << m_refineCount << "face=" << m_faceDetail;

    setErrorMessage(QString());
    setStatusMessage(tr("Generating..."));
    m_runningAction = ComfyPilotDefaults::actionGenerate();
    launchJob(buildJob());
}

/**
 * @brief Starts an asynchronous image-to-video generation from an input image.
 * @param inputPath Local path of the image displayed in the Comfy view.
 */
void ComfyPilotController::generateVideo(const QString &inputPath)
{
    if (m_running) {
        qWarning() << "ComfyPilot video generate rejected: already running";
        return;
    }
    if (!m_videoEnabled) {
        qWarning() << "ComfyPilot video generate rejected: video mode disabled";
        return;
    }
    if (!m_useCurrentImage) {
        qWarning() << "ComfyPilot video generate rejected: input image not enabled";
        setErrorMessage(tr("Enable \"Use current image\" to generate video"));
        return;
    }
    const QFileInfo inputInfo(inputPath);
    if (inputPath.trimmed().isEmpty() || !inputInfo.exists() || !inputInfo.isFile()) {
        qWarning() << "ComfyPilot video generate rejected: missing input image";
        setErrorMessage(tr("Select an input image to generate video"));
        return;
    }
    if (VideoThumbnailUtils::isVideoFile(inputInfo)) {
        qWarning() << "ComfyPilot video generate rejected: input is a video";
        setErrorMessage(tr("Video input must be an image"));
        return;
    }
    qInfo() << "ComfyPilot video generate requested:"
            << "input=" << inputPath
            << "duration=" << m_videoDuration
            << "rate=" << m_videoFrameRate;

    setErrorMessage(QString());
    setStatusMessage(tr("Generating video..."));
    m_runningAction = ComfyPilotDefaults::actionGenerate();
    ComfyPilotJob job = buildJob();
    job.videoInputPath = inputPath;
    launchJob(job);
}

/**
 * @brief Starts a fast preview generation without refinement or face detail.
 */
void ComfyPilotController::preview()
{
    if (m_running) {
        qWarning() << "ComfyPilot preview rejected: already running";
        return;
    }
    if (m_videoEnabled) {
        qWarning() << "ComfyPilot preview rejected: unavailable in video mode";
        return;
    }
    qInfo() << "ComfyPilot preview requested:"
            << "canvas=" << m_canvasWidth << "x" << m_canvasHeight;

    setErrorMessage(QString());
    setStatusMessage(tr("Generating preview..."));
    ComfyPilotJob job = buildJob();
    job.parameters.refineCount = 0;
    job.parameters.faceDetail = false;
    m_runningAction = ComfyPilotDefaults::actionPreview();
    launchJob(job);
}

/**
 * @brief Increments the seed then starts a fast preview generation.
 */
void ComfyPilotController::previewNextSeed()
{
    if (m_running) {
        qWarning() << "ComfyPilot preview next seed rejected: already running";
        return;
    }
    if (m_videoEnabled) {
        qWarning() << "ComfyPilot preview next seed rejected: unavailable in video mode";
        return;
    }
    setSeed(m_seed + 1);
    qInfo() << "ComfyPilot preview next seed requested: seed=" << m_seed;
    setErrorMessage(QString());
    setStatusMessage(tr("Generating preview..."));
    ComfyPilotJob job = buildJob();
    job.parameters.refineCount = 0;
    job.parameters.faceDetail = false;
    m_runningAction = ComfyPilotDefaults::actionNextSeed();
    launchJob(job);
}

/**
 * @brief Requests cancellation and sends an interrupt to ComfyUI.
 */
void ComfyPilotController::cancel()
{
    if (!m_running) {
        return;
    }
    qInfo() << "ComfyPilot cancel requested";
    if (m_worker) {
        m_worker->cancel();
    }
    ComfyClient client;
    QString error;
    client.interrupt(m_serverUrl.isEmpty() ? ComfyPilotDefaults::serverUrl() : m_serverUrl, 5000, &error);
    if (!error.isEmpty()) {
        qWarning() << "ComfyPilot interrupt failed:" << error;
    }
    setStatusMessage(tr("Cancelling..."));
}

/**
 * @brief Loads persisted pilot parameters from settings storage.
 */
void ComfyPilotController::loadParameters()
{
    ApplicationSettings settings;
    settings.beginGroup(settingsGroup);
    m_serverUrl = settings.value(QStringLiteral("serverUrl"), m_serverUrl).toString();
    m_positivePrompt = settings.value(QStringLiteral("positivePrompt"), m_positivePrompt).toString();
    m_negativePrompt = settings.value(QStringLiteral("negativePrompt"), m_negativePrompt).toString();
    m_canvasWidth = settings.value(QStringLiteral("canvasWidth"), m_canvasWidth).toInt();
    m_canvasHeight = settings.value(QStringLiteral("canvasHeight"), m_canvasHeight).toInt();
    m_refineCount = settings.value(QStringLiteral("refineCount"), m_refineCount).toInt();
    m_faceDetail = settings.value(QStringLiteral("faceDetail"), m_faceDetail).toBool();
    m_emptyRefinePrompt = settings.value(QStringLiteral("emptyRefinePrompt"), m_emptyRefinePrompt).toBool();
    m_seed = settings.value(QStringLiteral("seed"), m_seed).toInt();
    m_initialSteps = settings.value(QStringLiteral("initialSteps"), m_initialSteps).toInt();
    m_refineSteps = settings.value(QStringLiteral("refineSteps"), m_refineSteps).toInt();
    m_initialGuidance = settings.value(QStringLiteral("initialGuidance"), m_initialGuidance).toDouble();
    m_refineGuidance = settings.value(QStringLiteral("refineGuidance"), m_refineGuidance).toDouble();
    m_initialDenoise = settings.value(QStringLiteral("initialDenoise"), m_initialDenoise).toDouble();
    m_refineDenoise = settings.value(QStringLiteral("refineDenoise"), m_refineDenoise).toDouble();
    m_videoEnabled = settings.value(QStringLiteral("videoEnabled"), m_videoEnabled).toBool();
    m_useCurrentImage = settings.value(QStringLiteral("useCurrentImage"), m_useCurrentImage).toBool();
    m_videoDuration = settings.value(QStringLiteral("videoDuration"), m_videoDuration).toInt();
    m_videoFrameRate = settings.value(QStringLiteral("videoFrameRate"), m_videoFrameRate).toInt();
    m_canvasSize = settings.value(QStringLiteral("canvasSize"), m_canvasSize).toInt();
    settings.endGroup();
    qInfo() << "ComfyPilot parameters loaded";
    restoreOutputPath();
}

/**
 * @brief Restores the last generated output so the view is populated at startup.
 *
 * Reuses the persisted output path when the file still exists, otherwise falls
 * back to the most recently modified media file in the default ComfyUI output
 * folder. Leaves the output empty when no media file is found.
 */
void ComfyPilotController::restoreOutputPath()
{
    ApplicationSettings settings;
    settings.beginGroup(settingsGroup);
    const QString savedPath = settings.value(QString::fromLatin1(settingsOutputPath)).toString();
    settings.endGroup();
    const QFileInfo savedInfo(savedPath);
    if (isMediaOutputInfo(savedInfo)) {
        qInfo() << "ComfyPilot restored saved output:" << savedInfo.absoluteFilePath();
        m_outputPath = savedInfo.absoluteFilePath();
        m_outputIsVideo = VideoThumbnailUtils::isVideoFile(savedInfo);
        return;
    }
    const QString latestPath = findLatestMediaOutput(PlatformUtils::comfyDefaultOutputDir());
    if (latestPath.isEmpty()) {
        qInfo() << "ComfyPilot no previous output found";
        return;
    }
    qInfo() << "ComfyPilot restored latest output:" << latestPath;
    m_outputPath = latestPath;
    m_outputIsVideo = VideoThumbnailUtils::isVideoFile(QFileInfo(latestPath));
}

/**
 * @brief Saves the current pilot parameters into settings storage.
 */
void ComfyPilotController::saveParameters() const
{
    ApplicationSettings settings;
    settings.beginGroup(settingsGroup);
    settings.setValue(QStringLiteral("serverUrl"), m_serverUrl);
    settings.setValue(QStringLiteral("positivePrompt"), m_positivePrompt);
    settings.setValue(QStringLiteral("negativePrompt"), m_negativePrompt);
    settings.setValue(QStringLiteral("canvasWidth"), m_canvasWidth);
    settings.setValue(QStringLiteral("canvasHeight"), m_canvasHeight);
    settings.setValue(QStringLiteral("refineCount"), m_refineCount);
    settings.setValue(QStringLiteral("faceDetail"), m_faceDetail);
    settings.setValue(QStringLiteral("emptyRefinePrompt"), m_emptyRefinePrompt);
    settings.setValue(QStringLiteral("seed"), m_seed);
    settings.setValue(QStringLiteral("initialSteps"), m_initialSteps);
    settings.setValue(QStringLiteral("refineSteps"), m_refineSteps);
    settings.setValue(QStringLiteral("initialGuidance"), m_initialGuidance);
    settings.setValue(QStringLiteral("refineGuidance"), m_refineGuidance);
    settings.setValue(QStringLiteral("initialDenoise"), m_initialDenoise);
    settings.setValue(QStringLiteral("refineDenoise"), m_refineDenoise);
    settings.setValue(QStringLiteral("videoEnabled"), m_videoEnabled);
    settings.setValue(QStringLiteral("useCurrentImage"), m_useCurrentImage);
    settings.setValue(QStringLiteral("videoDuration"), m_videoDuration);
    settings.setValue(QStringLiteral("videoFrameRate"), m_videoFrameRate);
    settings.setValue(QStringLiteral("canvasSize"), m_canvasSize);
    settings.endGroup();
    settings.sync();
}

/**
 * @brief Imports pilot parameters from the workflow embedded in an image.
 * @param imagePath Image file path to read the workflow from.
 * @return Result map with ok, filled control names, and error fields.
 */
QVariantMap ComfyPilotController::importFromImage(const QString &imagePath)
{
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);

    qInfo() << "ComfyPilot import requested:" << imagePath;
    QString error;
    const QJsonObject prompt = ComfyPromptImporter::readEmbeddedPrompt(imagePath, &error);
    if (!error.isEmpty() || prompt.isEmpty()) {
        qWarning() << "ComfyPilot import failed:" << error;
        setErrorMessage(error.isEmpty() ? tr("Import failed") : error);
        result.insert(QStringLiteral("error"), error.isEmpty() ? tr("Import failed") : error);
        return result;
    }

    ComfyPilotParameters parameters = buildJob().parameters;
    const QStringList filled = ComfyPromptImporter::extractParameters(prompt, &parameters, &error);
    if (!error.isEmpty() || filled.isEmpty()) {
        qWarning() << "ComfyPilot import failed:" << error;
        setErrorMessage(error.isEmpty() ? tr("Import failed") : error);
        result.insert(QStringLiteral("error"), error.isEmpty() ? tr("Import failed") : error);
        return result;
    }

    setPositivePrompt(parameters.positivePrompt);
    setNegativePrompt(parameters.negativePrompt);
    setCanvasWidth(parameters.canvasWidth);
    setCanvasHeight(parameters.canvasHeight);
    setRefineCount(parameters.refineCount);
    setFaceDetail(parameters.faceDetail);
    setEmptyRefinePrompt(parameters.emptyRefinePrompt);
    setSeed(parameters.seed);
    setInitialSteps(parameters.initialSteps);
    setRefineSteps(parameters.refineSteps);
    setInitialGuidance(parameters.initialGuidance);
    setRefineGuidance(parameters.refineGuidance);
    setInitialDenoise(parameters.initialDenoise);
    setRefineDenoise(parameters.refineDenoise);

    qInfo() << "ComfyPilot import done, filled controls:" << filled;
    setErrorMessage(QString());
    setStatusMessage(tr("Workflow imported"));
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("filled"), filled);
    return result;
}

/**
 * @brief Builds a generation job from the current controller properties.
 * @return Job with server URL and parameters copied from the controller.
 */
ComfyPilotJob ComfyPilotController::buildJob() const
{
    ComfyPilotJob job;
    job.serverUrl = m_serverUrl;
    job.parameters.canvasWidth = m_canvasWidth;
    job.parameters.canvasHeight = m_canvasHeight;
    job.parameters.refineCount = m_refineCount;
    job.parameters.faceDetail = m_faceDetail;
    job.parameters.emptyRefinePrompt = m_emptyRefinePrompt;
    job.parameters.seed = m_seed;
    job.parameters.initialSteps = m_initialSteps;
    job.parameters.refineSteps = m_refineSteps;
    job.parameters.initialGuidance = m_initialGuidance;
    job.parameters.refineGuidance = m_refineGuidance;
    job.parameters.initialDenoise = m_initialDenoise;
    job.parameters.refineDenoise = m_refineDenoise;
    job.parameters.positivePrompt = m_positivePrompt;
    job.parameters.negativePrompt = m_negativePrompt;
    job.parameters.videoEnabled = m_videoEnabled;
    job.parameters.useCurrentImage = m_useCurrentImage;
    job.parameters.videoDuration = m_videoDuration;
    job.parameters.videoFrameRate = m_videoFrameRate;
    job.parameters.canvasSize = m_canvasSize;
    return job;
}

/**
 * @brief Launches the given job in a worker thread.
 * @param job Generation job to run.
 */
void ComfyPilotController::launchJob(const ComfyPilotJob &job)
{
    cleanupThread();
    m_thread = new QThread(this);
    m_worker = new ComfyPilotWorker(job);
    m_worker->moveToThread(m_thread);
    connect(m_thread, &QThread::started, m_worker, &ComfyPilotWorker::start);
    connect(m_worker, &ComfyPilotWorker::finished, this, &ComfyPilotController::handleWorkerFinished);
    connect(m_worker, &ComfyPilotWorker::finished, m_thread, &QThread::quit);
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(m_thread, &QThread::finished, m_thread, &QObject::deleteLater);
    setRunning(true);
    m_thread->start();
}

void ComfyPilotController::handleWorkerFinished(QVariantMap result)
{
    m_runningAction.clear();
    const bool ok = result.value(QStringLiteral("ok")).toBool();
    if (ok) {
        const QString output = result.value(QStringLiteral("output")).toString();
        const bool isVideo = result.value(QStringLiteral("isVideo")).toBool();
        qInfo() << "ComfyPilot generation finished:" << output;
        setOutputPath(output, isVideo);
        setStatusMessage(tr("Done"));
    } else {
        const QString errorText = result.value(QStringLiteral("error")).toString();
        qWarning() << "ComfyPilot generation failed:" << errorText;
        setErrorMessage(errorText.isEmpty() ? tr("Generation failed") : errorText);
        setStatusMessage(tr("Failed"));
    }
    m_worker = nullptr;
    m_thread = nullptr;
    setRunning(false);
}

void ComfyPilotController::setRunning(bool value)
{
    if (m_running == value) {
        return;
    }
    m_running = value;
    emit runningChanged();
}

void ComfyPilotController::setOutputPath(const QString &value, bool isVideo)
{
    m_outputPath = value;
    m_outputIsVideo = isVideo;
    ApplicationSettings settings;
    settings.beginGroup(settingsGroup);
    settings.setValue(QString::fromLatin1(settingsOutputPath), m_outputPath);
    settings.endGroup();
    settings.sync();
    emit outputPathChanged();
}

void ComfyPilotController::setErrorMessage(const QString &value)
{
    if (m_errorMessage == value) {
        return;
    }
    m_errorMessage = value;
    emit errorMessageChanged();
}

void ComfyPilotController::setStatusMessage(const QString &value)
{
    if (m_statusMessage == value) {
        return;
    }
    m_statusMessage = value;
    emit statusMessageChanged();
}

void ComfyPilotController::cleanupThread()
{
    if (m_thread && m_thread->isRunning()) {
        return;
    }
    m_thread = nullptr;
    m_worker = nullptr;
}
