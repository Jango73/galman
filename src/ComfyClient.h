#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantMap>

class ComfyClient : public QObject
{
    Q_OBJECT

public:
    explicit ComfyClient(QObject *parent = nullptr);

    QVariantMap runWorkflow(const QString &serverUrl,
                            const QJsonObject &promptPayload,
                            const QString &clientId,
                            int timeoutMs,
                            int pollIntervalMs,
                            int maxPollMs);
    bool downloadImage(const QString &serverUrl,
                       const QString &filename,
                       const QString &subfolder,
                       const QString &type,
                       const QString &targetPath,
                       int timeoutMs,
                       QString *error);

private:
    QJsonObject postJson(const QString &url, const QJsonObject &payload, int timeoutMs, QString *error);
    QJsonObject getJson(const QString &url, int timeoutMs, QString *error);
    QString extractStatus(const QJsonObject &payload) const;
};
