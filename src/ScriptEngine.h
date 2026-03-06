#pragma once

#include <QJSEngine>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class QProcess;

class ScriptEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList selection READ selection NOTIFY selectionChanged)
    Q_PROPERTY(bool processRunning READ processRunning NOTIFY processRunningChanged)
    Q_PROPERTY(qreal processProgress READ processProgress NOTIFY processProgressChanged)
    Q_PROPERTY(QString processStatusMessage READ processStatusMessage NOTIFY processStatusMessageChanged)

public:
    explicit ScriptEngine(QObject *parent = nullptr);

    QVariantList selection() const;
    bool processRunning() const;
    qreal processProgress() const;
    QString processStatusMessage() const;

    Q_INVOKABLE void setSelection(const QStringList &paths);
    Q_INVOKABLE QVariant run(const QString &script);
    Q_INVOKABLE QVariant runFile(const QString &path);
    Q_INVOKABLE QVariantMap renameFile(const QString &from, const QString &to);
    Q_INVOKABLE QVariantMap moveToTrash(const QString &path);
    Q_INVOKABLE QVariantMap loadScript(const QString &path);
    Q_INVOKABLE QVariant runScript(const QString &path, const QVariantMap &params);
    Q_INVOKABLE QStringList supportedImageFormats() const;
    Q_INVOKABLE QVariantMap saveAdjustedImage(const QImage &image, const QString &path);
    Q_INVOKABLE QVariantMap convertImage(const QString &path, const QString &format);
    Q_INVOKABLE QVariantMap runComfyWorkflow(const QString &workflowPath,
                                             const QString &imagePath,
                                             const QString &serverUrl,
                                             const QString &outputDir);
    Q_INVOKABLE QVariantMap runProcess(const QString &program,
                                       const QStringList &arguments,
                                       const QString &workingFolder = QString());
    Q_INVOKABLE void cancelRunningProcess();
    Q_INVOKABLE QString comfyDefaultOutputDir() const;
    Q_INVOKABLE QVariantMap loadScriptParams(const QString &scriptPath);
    Q_INVOKABLE void saveScriptParams(const QString &scriptPath, const QVariantMap &params);

signals:
    void selectionChanged();
    void processRunningChanged();
    void processProgressChanged();
    void processStatusMessageChanged();

private:
    void updateEngineSelection();
    void setProcessRunning(bool running);
    void setProcessProgress(qreal value);
    void setProcessStatusMessage(const QString &message);

    QJSEngine m_engine;
    QVariantList m_selection;
    bool m_processRunning = false;
    qreal m_processProgress = 0.0;
    QString m_processStatusMessage;
    QProcess *m_runningProcess = nullptr;
};
