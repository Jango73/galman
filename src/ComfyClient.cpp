
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

#include "ComfyClient.h"

#include <QEventLoop>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QThread>
#include <QUrlQuery>
#include <QFile>
#include <QDir>

/**
 * @brief Builds a client instance bound to the given QObject parent.
 * @param parent Parent QObject for ownership.
 */
ComfyClient::ComfyClient(QObject *parent)
    : QObject(parent)
{
}

/**
 * @brief Submits a workflow prompt to ComfyUI and polls until completion or timeout.
 * @param serverUrl Base URL of the ComfyUI server.
 * @param promptPayload JSON payload describing the workflow prompt.
 * @param clientId Client identifier forwarded to the server.
 * @param timeoutMs Timeout in milliseconds for each HTTP request.
 * @param pollIntervalMs Delay in milliseconds between polling attempts.
 * @param maxPollMs Maximum total polling time in milliseconds.
 * @return A map with ok=true and data on success, or ok=false and error on failure.
 */
QVariantMap ComfyClient::runWorkflow(const QString &serverUrl,
                                     const QJsonObject &promptPayload,
                                     const QString &clientId,
                                     int timeoutMs,
                                     int pollIntervalMs,
                                     int maxPollMs)
{
    QVariantMap result;
    result.insert("ok", false);

    const QString baseUrl = serverUrl.endsWith("/") ? serverUrl.left(serverUrl.size() - 1) : serverUrl;
    const QString promptUrl = baseUrl + "/prompt";

    QJsonObject payload = promptPayload;
    payload.insert("client_id", clientId);

    QString error;
    const QJsonObject response = postJson(promptUrl, payload, timeoutMs, &error);
    if (!error.isEmpty()) {
        result.insert("error", error);
        return result;
    }

    const QString promptId = response.value("prompt_id").toString();
    if (promptId.isEmpty()) {
        result.insert("error", "ComfyUI did not return a prompt_id");
        return result;
    }

    const QString historyUrl = baseUrl + "/history/" + promptId;
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < maxPollMs) {
        const QJsonObject raw = getJson(historyUrl, timeoutMs, &error);
        if (!error.isEmpty()) {
            if (error.contains("404")) {
                QThread::msleep(pollIntervalMs);
                continue;
            }
            result.insert("error", error);
            return result;
        }

        QJsonObject data = raw;
        if (raw.contains(promptId) && raw.value(promptId).isObject()) {
            data = raw.value(promptId).toObject();
            if (!data.contains("prompt_id")) {
                data.insert("prompt_id", promptId);
            }
        }

        const QString status = extractStatus(data);
        if (status == "done") {
            result.insert("ok", true);
            result.insert("data", data.toVariantMap());
            return result;
        }
        if (status == "error") {
            result.insert("error", "ComfyUI reported an error");
            return result;
        }

        QThread::msleep(pollIntervalMs);
    }

    result.insert("error", "Timeout while waiting for result");
    return result;
}

/**
 * @brief Downloads an image from the ComfyUI view endpoint into a local file.
 * @param serverUrl Base URL of the ComfyUI server.
 * @param filename Image filename on the server.
 * @param subfolder Optional server subfolder for the image.
 * @param type Optional ComfyUI type parameter.
 * @param targetPath Destination file path on disk.
 * @param timeoutMs Timeout in milliseconds for the HTTP request.
 * @param error Optional output error message.
 * @return True on successful download and write, false otherwise.
 */
bool ComfyClient::downloadImage(const QString &serverUrl,
                                const QString &filename,
                                const QString &subfolder,
                                const QString &type,
                                const QString &targetPath,
                                int timeoutMs,
                                QString *error)
{
    if (filename.isEmpty()) {
        if (error) {
            *error = "Missing filename for download";
        }
        return false;
    }

    QUrl baseUrl(serverUrl);
    const QString cleanBase = baseUrl.toString().endsWith("/")
        ? baseUrl.toString().left(baseUrl.toString().size() - 1)
        : baseUrl.toString();
    QUrl viewUrl(cleanBase + "/view");
    QUrlQuery query;
    query.addQueryItem("filename", filename);
    if (!subfolder.isEmpty()) {
        query.addQueryItem("subfolder", subfolder);
    }
    if (!type.isEmpty()) {
        query.addQueryItem("type", type);
    }
    viewUrl.setQuery(query);

    QNetworkAccessManager manager;
    QNetworkRequest request{viewUrl};

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!reply->isFinished() || timer.isActive() == false) {
        if (error) {
            *error = "HTTP timeout while downloading image";
        }
        reply->deleteLater();
        return false;
    }

    if (reply->error() != QNetworkReply::NoError) {
        if (error) {
            *error = QString("HTTP error: %1").arg(reply->errorString());
        }
        reply->deleteLater();
        return false;
    }

    const QByteArray body = reply->readAll();
    reply->deleteLater();

    const QFileInfo targetInfo(targetPath);
    QDir().mkpath(targetInfo.absolutePath());
    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = QString("Failed to write output file: %1").arg(targetPath);
        }
        return false;
    }
    file.write(body);
    file.close();
    return true;
}

/**
 * @brief Sends a JSON payload via HTTP POST and returns the JSON response.
 * @param url Target URL to post to.
 * @param payload JSON payload to send.
 * @param timeoutMs Timeout in milliseconds for the HTTP request.
 * @param error Optional output error message.
 * @return Parsed JSON object response, or an empty object on failure.
 */
QJsonObject ComfyClient::postJson(const QString &url, const QJsonObject &payload, int timeoutMs, QString *error)
{
    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson());
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    QJsonObject response;
    if (!reply->isFinished() || timer.isActive() == false) {
        if (error) {
            *error = "HTTP timeout";
        }
    } else if (reply->error() != QNetworkReply::NoError) {
        if (error) {
            *error = QString("HTTP error: %1").arg(reply->errorString());
        }
    } else {
        const QByteArray body = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (doc.isObject()) {
            response = doc.object();
        }
    }

    reply->deleteLater();
    return response;
}

/**
 * @brief Performs an HTTP GET and returns the JSON response body.
 * @param url Target URL to request.
 * @param timeoutMs Timeout in milliseconds for the HTTP request.
 * @param error Optional output error message.
 * @return Parsed JSON object response, or an empty object on failure.
 */
QJsonObject ComfyClient::getJson(const QString &url, int timeoutMs, QString *error)
{
    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(url)};

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    QJsonObject response;
    if (!reply->isFinished() || timer.isActive() == false) {
        if (error) {
            *error = "HTTP timeout";
        }
    } else if (reply->error() != QNetworkReply::NoError) {
        if (error) {
            *error = QString("HTTP %1").arg(reply->errorString());
        }
    } else {
        const QByteArray body = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (doc.isObject()) {
            response = doc.object();
        }
    }

    reply->deleteLater();
    return response;
}

/**
 * @brief Normalizes ComfyUI status payloads into done, error, or pending.
 * @param payload JSON status payload from the server.
 * @return Normalized status string.
 */
QString ComfyClient::extractStatus(const QJsonObject &payload) const
{
    const QJsonObject statusBlock = payload.value("status").toObject();
    const QString statusValue = statusBlock.value("status").toString().toLower();
    if (statusValue == "success" || statusValue == "ok" || statusValue == "completed") {
        return "done";
    }
    if (statusValue == "error" || statusValue == "failed") {
        return "error";
    }
    if (payload.contains("outputs")) {
        return "done";
    }
    return "pending";
}
