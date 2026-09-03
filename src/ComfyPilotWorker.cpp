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

#include "ComfyPilotWorker.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QVariantList>

#include "ComfyClient.h"
#include "PlatformUtils.h"
#include "VideoThumbnailUtils.h"

namespace
{

struct PilotNetworkConstants
{
    static constexpr int requestTimeoutMs = 30000;
    static constexpr int pollIntervalMs = 1000;
    static constexpr int maxPollMs = 600000;
};

bool outputIsVideoPath(const QString &path)
{
    const QFileInfo info(path);
    if (VideoThumbnailUtils::isVideoFile(info)) {
        return true;
    }
    static QMimeDatabase mimeDatabase;
    const QMimeType mimeType = mimeDatabase.mimeTypeForFile(info, QMimeDatabase::MatchExtension);
    return mimeType.isValid() && mimeType.name().startsWith(QStringLiteral("video/"));
}

QVariantList outputEntriesFromData(const QVariantMap &data)
{
    const QVariant outputsValue = data.value(QStringLiteral("outputs"));
    if (outputsValue.typeId() == QMetaType::QVariantList) {
        return outputsValue.toList();
    }
    if (outputsValue.typeId() == QMetaType::QVariantMap) {
        return outputsValue.toMap().values();
    }
    return {};
}

QVariantList filesFromEntry(const QVariantMap &entry)
{
    static const QStringList fileKeys = {QStringLiteral("images"),
                                         QStringLiteral("gifs"),
                                         QStringLiteral("videos")};
    for (const QString &key : fileKeys) {
        const QVariant filesValue = entry.value(key);
        if (filesValue.typeId() == QMetaType::QVariantList) {
            const QVariantList files = filesValue.toList();
            if (!files.isEmpty()) {
                return files;
            }
        } else if (filesValue.typeId() == QMetaType::QVariantMap) {
            const QVariantList files = filesValue.toMap().values();
            if (!files.isEmpty()) {
                return files;
            }
        }
    }
    return {};
}

} // namespace

ComfyPilotWorker::ComfyPilotWorker(const ComfyPilotJob &job, QObject *parent)
    : QObject(parent)
    , m_job(job)
{
}

/**
 * @brief Cancels the running generation at the next polling step.
 */
void ComfyPilotWorker::cancel()
{
    m_cancelled.storeRelaxed(1);
}

/**
 * @brief Builds the prompt, submits it to ComfyUI, and downloads the first output.
 *
 * When the server answers from its cache with a reference to a deleted file,
 * the submission is retried once with a distinct save prefix, forcing the
 * output node out of the server cache so the file is actually regenerated.
 */
void ComfyPilotWorker::start()
{
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);

    QString serverUrl = m_job.serverUrl.trimmed();
    if (serverUrl.isEmpty()) {
        serverUrl = ComfyPilotDefaults::serverUrl();
    }
    qInfo() << "ComfyPilot generation started on" << serverUrl;

    QString buildError;
    const QJsonObject prompt = ComfyWorkflowBuilder::buildPrompt(m_job.parameters, &buildError);
    if (!buildError.isEmpty() || prompt.isEmpty()) {
        qWarning() << "ComfyPilot build failed:" << buildError;
        result.insert(QStringLiteral("error"), buildError.isEmpty() ? tr("Invalid parameters") : buildError);
        emit finished(result);
        return;
    }

    QVariantMap comfyResult = submitPrompt(serverUrl, prompt);
    if (!comfyResult.value(QStringLiteral("ok")).toBool()) {
        result.insert(QStringLiteral("error"), comfyResult.value(QStringLiteral("error")));
        emit finished(result);
        return;
    }
    if (resolveOutput(serverUrl, comfyResult, &result)) {
        emit finished(result);
        return;
    }
    if (m_cancelled.loadRelaxed() != 0) {
        result.insert(QStringLiteral("error"), tr("Cancelled by user"));
        emit finished(result);
        return;
    }

    qInfo() << "ComfyPilot output missing, forcing re-execution with prefix"
            << ComfyWorkflowBuilder::retrySavePrefix();
    const QJsonObject retryPrompt = ComfyWorkflowBuilder::buildPrompt(
        m_job.parameters, &buildError, ComfyWorkflowBuilder::retrySavePrefix());
    if (!buildError.isEmpty() || retryPrompt.isEmpty()) {
        qWarning() << "ComfyPilot retry build failed:" << buildError;
        emit finished(result);
        return;
    }

    comfyResult = submitPrompt(serverUrl, retryPrompt);
    if (!comfyResult.value(QStringLiteral("ok")).toBool()) {
        result.insert(QStringLiteral("error"), comfyResult.value(QStringLiteral("error")));
        emit finished(result);
        return;
    }
    if (resolveOutput(serverUrl, comfyResult, &result)) {
        emit finished(result);
        return;
    }

    if (m_cancelled.loadRelaxed() != 0) {
        result.insert(QStringLiteral("error"), tr("Cancelled by user"));
    } else if (!result.contains(QStringLiteral("error"))) {
        result.insert(QStringLiteral("error"), tr("No output file found"));
    }
    qWarning() << "ComfyPilot no output:" << result.value(QStringLiteral("error")).toString();
    emit finished(result);
}

/**
 * @brief Submits a prompt payload and polls until completion, timeout, or cancel.
 * @param serverUrl Base URL of the ComfyUI server.
 * @param prompt Prompt object in ComfyUI API format.
 * @return A map with ok=true and data on success, or ok=false and error on failure.
 */
QVariantMap ComfyPilotWorker::submitPrompt(const QString &serverUrl, const QJsonObject &prompt)
{
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);

    QJsonObject payload;
    payload.insert(QStringLiteral("prompt"), prompt);
    qInfo() << "ComfyPilot prompt payload:"
            << QString::fromUtf8(QJsonDocument(prompt).toJson(QJsonDocument::Compact));

    ComfyClient client;
    QVariantMap comfyResult = client.runWorkflowCancellable(
        serverUrl, payload, QStringLiteral("galman"),
        PilotNetworkConstants::requestTimeoutMs,
        PilotNetworkConstants::pollIntervalMs,
        PilotNetworkConstants::maxPollMs,
        [this]() { return m_cancelled.loadRelaxed() != 0; });

    if (!comfyResult.value(QStringLiteral("ok")).toBool()) {
        qWarning() << "ComfyPilot run failed:" << comfyResult.value(QStringLiteral("error")).toString();
        result.insert(QStringLiteral("error"), comfyResult.value(QStringLiteral("error")));
        return result;
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("data"), comfyResult.value(QStringLiteral("data")));
    return result;
}

/**
 * @brief Resolves the first output file of a completed run into a local path.
 * @param serverUrl Base URL of the ComfyUI server used for downloads.
 * @param comfyResult Successful submission result holding the history data.
 * @param result Output result map filled with ok, output, and isVideo on success.
 * @return True when an output file was resolved, false otherwise.
 */
bool ComfyPilotWorker::resolveOutput(const QString &serverUrl,
                                     const QVariantMap &comfyResult,
                                     QVariantMap *result)
{
    const QVariantMap data = comfyResult.value(QStringLiteral("data")).toMap();
    const QVariantList entries = outputEntriesFromData(data);
    const QString resolvedOutputDir = PlatformUtils::comfyDefaultOutputDir();
    const QVariantMap meta = data.value(QStringLiteral("meta")).toMap();
    const QString serverOutputDir = meta.value(QStringLiteral("paths")).toMap().value(QStringLiteral("output")).toString();

    ComfyClient client;
    QString lastDownloadError;
    for (const QVariant &entryVar : entries) {
        const QVariantList files = filesFromEntry(entryVar.toMap());
        if (files.isEmpty()) {
            continue;
        }
        const QVariantMap fileMeta = files.first().toMap();
        const QString filename = fileMeta.value(QStringLiteral("filename")).toString();
        const QString subfolder = fileMeta.value(QStringLiteral("subfolder")).toString();
        const QString fileType = fileMeta.value(QStringLiteral("type")).toString();
        if (filename.isEmpty()) {
            continue;
        }
        if (QFileInfo(filename).isAbsolute() && QFileInfo::exists(filename)) {
            qInfo() << "ComfyPilot using absolute output:" << filename;
            result->remove(QStringLiteral("error"));
            result->insert(QStringLiteral("ok"), true);
            result->insert(QStringLiteral("output"), filename);
            result->insert(QStringLiteral("isVideo"), outputIsVideoPath(filename));
            return true;
        }
        QString relativePath = filename;
        if (!subfolder.isEmpty()) {
            relativePath = QDir(subfolder).filePath(filename);
        }
        QString candidate = QDir(resolvedOutputDir).filePath(relativePath);
        if (!QFileInfo::exists(candidate)) {
            candidate.clear();
        }
        if (candidate.isEmpty() && !serverOutputDir.isEmpty() && serverOutputDir != resolvedOutputDir) {
            const QString serverCandidate = QDir(serverOutputDir).filePath(relativePath);
            if (QFileInfo::exists(serverCandidate)) {
                candidate = serverCandidate;
            }
        }
        if (!candidate.isEmpty()) {
            qInfo() << "ComfyPilot using local output:" << candidate;
            result->remove(QStringLiteral("error"));
            result->insert(QStringLiteral("ok"), true);
            result->insert(QStringLiteral("output"), candidate);
            result->insert(QStringLiteral("isVideo"), outputIsVideoPath(candidate));
            return true;
        }
        const QString targetPath = QDir(resolvedOutputDir).filePath(relativePath);
        QString downloadError;
        if (client.downloadOutput(serverUrl, filename, subfolder, fileType, targetPath,
                                  PilotNetworkConstants::requestTimeoutMs, &downloadError)) {
            qInfo() << "ComfyPilot downloaded output:" << targetPath;
            result->remove(QStringLiteral("error"));
            result->insert(QStringLiteral("ok"), true);
            result->insert(QStringLiteral("output"), targetPath);
            result->insert(QStringLiteral("isVideo"), outputIsVideoPath(targetPath));
            return true;
        }
        if (!downloadError.isEmpty()) {
            lastDownloadError = downloadError;
        }
    }

    if (!lastDownloadError.isEmpty()) {
        result->insert(QStringLiteral("error"), lastDownloadError);
    } else {
        result->insert(QStringLiteral("error"), tr("No output file found"));
    }
    return false;
}
