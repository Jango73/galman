#pragma once

#include <QAtomicInt>
#include <QObject>
#include <QString>
#include <QVariantMap>

#include "ComfyWorkflowBuilder.h"

struct ComfyPilotJob
{
    QString serverUrl;
    ComfyPilotParameters parameters;
};

class ComfyPilotWorker : public QObject
{
    Q_OBJECT

public:
    explicit ComfyPilotWorker(const ComfyPilotJob &job, QObject *parent = nullptr);

public slots:
    void start();
    void cancel();

signals:
    void finished(QVariantMap result);

private:
    QVariantMap submitPrompt(const QString &serverUrl, const QJsonObject &prompt);
    bool resolveOutput(const QString &serverUrl,
                       const QVariantMap &comfyResult,
                       QVariantMap *result);

    ComfyPilotJob m_job;
    QAtomicInt m_cancelled = 0;
};
