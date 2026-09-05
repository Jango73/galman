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

#include "ComfyPrerequisites.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

#include "ComfyRequirements.h"
#include "ComfyWorkflowBuilder.h"
#include "PlatformUtils.h"

namespace
{

struct NetworkLimits
{
    static constexpr int requestTimeoutMs = 15000;
};

const char statusInstalledLiteral[] = "installed";
const char statusMissingLiteral[] = "missing";
const char statusUnknownLiteral[] = "unknown";
const char objectInfoPath[] = "/object_info";

QString statusForType(const QString &classType, const QSet<QString> &availableTypes)
{
    if (availableTypes.contains(classType)) {
        return QString::fromLatin1(statusInstalledLiteral);
    }
    return QString::fromLatin1(statusMissingLiteral);
}

} // namespace

ComfyPrerequisites::ComfyPrerequisites(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_nodeStatuses(buildUnknownStatuses())
    , m_modelRequirements(buildUnknownModelStatuses())
{
    qInfo() << "ComfyPrerequisites created, models=" << m_modelRequirements.size()
            << "nodes=" << m_nodeStatuses.size();
}

ComfyPrerequisites::~ComfyPrerequisites()
{
    if (m_pendingReply) {
        m_pendingReply->abort();
    }
}

bool ComfyPrerequisites::checking() const
{
    return m_checking;
}

QVariantList ComfyPrerequisites::nodeStatuses() const
{
    return m_nodeStatuses;
}

QVariantList ComfyPrerequisites::modelRequirements() const
{
    return m_modelRequirements;
}

QString ComfyPrerequisites::statusMessage() const
{
    return m_statusMessage;
}

QString ComfyPrerequisites::errorMessage() const
{
    return m_errorMessage;
}

QString ComfyPrerequisites::statusInstalled() const
{
    return QString::fromLatin1(statusInstalledLiteral);
}

QString ComfyPrerequisites::statusMissing() const
{
    return QString::fromLatin1(statusMissingLiteral);
}

QString ComfyPrerequisites::statusUnknown() const
{
    return QString::fromLatin1(statusUnknownLiteral);
}

/**
 * @brief Starts an asynchronous check of required nodes against the server.
 * @param serverUrl Base URL of the ComfyUI server, defaults when empty.
 */
void ComfyPrerequisites::refresh(const QString &serverUrl)
{
    if (m_checking) {
        qWarning() << "ComfyPrerequisites refresh rejected: already checking";
        return;
    }
    const QString baseUrl = normalizedServerUrl(serverUrl);
    qInfo() << "ComfyPrerequisites refresh requested:" << baseUrl;
    setErrorMessage(QString());
    setStatusMessage(tr("Checking prerequisites..."));
    setModelRequirements(buildModelStatuses(baseUrl, PlatformUtils::comfyModelsFolder()));
    setChecking(true);

    QNetworkRequest request(QUrl(baseUrl + QString::fromLatin1(objectInfoPath)));
    QNetworkReply *reply = m_networkManager->get(request);
    m_pendingReply = reply;
    QTimer::singleShot(NetworkLimits::requestTimeoutMs, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, &ComfyPrerequisites::handleReplyFinished);
}

/**
 * @brief Opens the ComfyUI server address in the default browser.
 * @param serverUrl Base URL of the ComfyUI server, defaults when empty.
 */
void ComfyPrerequisites::openManager(const QString &serverUrl)
{
    const QString baseUrl = normalizedServerUrl(serverUrl);
    qInfo() << "ComfyPrerequisites open manager requested:" << baseUrl;
    QDesktopServices::openUrl(QUrl(baseUrl));
}

/**
 * @brief Reports whether a server URL targets this machine.
 * @param serverUrl Server URL to inspect, defaults when empty.
 * @return True for loopback or empty URLs, false for remote hosts.
 */
bool ComfyPrerequisites::isLocalServer(const QString &serverUrl)
{
    return isLocalServerUrl(normalizedServerUrl(serverUrl));
}

QVariantList ComfyPrerequisites::buildUnknownStatuses()
{
    QVariantList statuses;
    const QVariantList entries = ComfyRequirements::nodeEntries();
    statuses.reserve(entries.size());
    for (const QVariant &entryVar : entries) {
        QVariantMap item = entryVar.toMap();
        item.insert(QStringLiteral("status"), QString::fromLatin1(statusUnknownLiteral));
        statuses.append(item);
    }
    return statuses;
}

QVariantList ComfyPrerequisites::buildNodeStatuses(const QSet<QString> &availableTypes)
{
    QVariantList statuses;
    const QVariantList entries = ComfyRequirements::nodeEntries();
    statuses.reserve(entries.size());
    for (const QVariant &entryVar : entries) {
        QVariantMap item = entryVar.toMap();
        item.insert(QStringLiteral("status"),
                    statusForType(item.value(QStringLiteral("classType")).toString(), availableTypes));
        statuses.append(item);
    }
    return statuses;
}

QVariantList ComfyPrerequisites::buildUnknownModelStatuses()
{
    QVariantList statuses;
    const QVariantList entries = ComfyRequirements::modelEntries();
    statuses.reserve(entries.size());
    for (const QVariant &entryVar : entries) {
        QVariantMap item = entryVar.toMap();
        item.insert(QStringLiteral("status"), QString::fromLatin1(statusUnknownLiteral));
        statuses.append(item);
    }
    return statuses;
}

/**
 * @brief Builds model statuses by scanning the local models folder.
 * @param serverUrl Normalized ComfyUI server URL used to detect remote setups.
 * @param modelsFolderPath Local ComfyUI models folder path to scan.
 * @return Model entries with installed, missing, or unknown status.
 */
QVariantList ComfyPrerequisites::buildModelStatuses(const QString &serverUrl,
                                                   const QString &modelsFolderPath)
{
    QVariantList statuses;
    const QVariantList entries = ComfyRequirements::modelEntries();
    statuses.reserve(entries.size());
    const bool canInspectLocal = isLocalServerUrl(serverUrl) && !modelsFolderPath.trimmed().isEmpty();
    QSet<QString> availableFiles;
    if (canInspectLocal) {
        QDirIterator scan(modelsFolderPath,
                          QDir::Files | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
        while (scan.hasNext()) {
            scan.next();
            if (scan.fileInfo().size() > 0) {
                availableFiles.insert(scan.fileName().toLower());
            }
        }
        qInfo() << "ComfyPrerequisites scanned models folder:" << modelsFolderPath
                << "files=" << availableFiles.size();
    } else {
        qInfo() << "ComfyPrerequisites model check skipped, remote server or missing folder";
    }
    for (const QVariant &entryVar : entries) {
        QVariantMap item = entryVar.toMap();
        if (!canInspectLocal) {
            item.insert(QStringLiteral("status"), QString::fromLatin1(statusUnknownLiteral));
        } else if (availableFiles.contains(item.value(QStringLiteral("fileName")).toString().toLower())) {
            item.insert(QStringLiteral("status"), QString::fromLatin1(statusInstalledLiteral));
        } else {
            item.insert(QStringLiteral("status"), QString::fromLatin1(statusMissingLiteral));
        }
        statuses.append(item);
    }
    return statuses;
}

void ComfyPrerequisites::handleReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (reply == nullptr) {
        return;
    }
    m_pendingReply.clear();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "ComfyPrerequisites check failed:" << reply->errorString();
        setStatusMessage(QString());
        setErrorMessage(tr("Cannot reach ComfyUI server"));
        setChecking(false);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "ComfyPrerequisites invalid response:" << parseError.errorString();
        setStatusMessage(QString());
        setErrorMessage(tr("Invalid response from ComfyUI server"));
        setChecking(false);
        return;
    }

    QSet<QString> availableTypes;
    const QJsonObject root = document.object();
    for (auto it = root.begin(); it != root.end(); ++it) {
        availableTypes.insert(it.key());
    }
    setNodeStatuses(buildNodeStatuses(availableTypes));

    int installedCount = 0;
    for (const QVariant &entryVar : m_nodeStatuses) {
        if (entryVar.toMap().value(QStringLiteral("status")).toString()
            == QString::fromLatin1(statusInstalledLiteral)) {
            ++installedCount;
        }
    }
    qInfo() << "ComfyPrerequisites check done, installed=" << installedCount << "of" << m_nodeStatuses.size();
    if (installedCount == m_nodeStatuses.size()) {
        setStatusMessage(tr("All required nodes are installed"));
    } else {
        setStatusMessage(tr("%1 of %2 nodes installed - install the missing ones from ComfyUI Manager")
                             .arg(installedCount)
                             .arg(m_nodeStatuses.size()));
    }
    setChecking(false);
}

QString ComfyPrerequisites::normalizedServerUrl(const QString &serverUrl)
{
    QString baseUrl = serverUrl.trimmed();
    if (baseUrl.isEmpty()) {
        baseUrl = ComfyPilotDefaults::serverUrl();
    }
    while (baseUrl.endsWith(QStringLiteral("/"))) {
        baseUrl.chop(1);
    }
    return baseUrl;
}

bool ComfyPrerequisites::isLocalServerUrl(const QString &serverUrl)
{
    const QString host = QUrl(serverUrl.trimmed().isEmpty() ? ComfyPilotDefaults::serverUrl() : serverUrl).host().toLower();
    return host.isEmpty() || host == QStringLiteral("localhost") || host == QStringLiteral("127.0.0.1")
        || host == QStringLiteral("::1");
}

void ComfyPrerequisites::setChecking(bool value)
{
    if (m_checking == value) {
        return;
    }
    m_checking = value;
    emit checkingChanged();
}

void ComfyPrerequisites::setNodeStatuses(const QVariantList &value)
{
    m_nodeStatuses = value;
    emit nodeStatusesChanged();
}

void ComfyPrerequisites::setModelRequirements(const QVariantList &value)
{
    m_modelRequirements = value;
    emit modelRequirementsChanged();
}

void ComfyPrerequisites::setStatusMessage(const QString &value)
{
    if (m_statusMessage == value) {
        return;
    }
    m_statusMessage = value;
    emit statusMessageChanged();
}

void ComfyPrerequisites::setErrorMessage(const QString &value)
{
    if (m_errorMessage == value) {
        return;
    }
    m_errorMessage = value;
    emit errorMessageChanged();
}
