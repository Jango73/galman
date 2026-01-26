#pragma once

#include <QJSEngine>
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class ScriptEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList selection READ selection NOTIFY selectionChanged)

public:
    explicit ScriptEngine(QObject *parent = nullptr);

    QVariantList selection() const;

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
    Q_INVOKABLE QString comfyDefaultOutputDir() const;
    Q_INVOKABLE QVariantMap loadScriptParams(const QString &scriptPath);
    Q_INVOKABLE void saveScriptParams(const QString &scriptPath, const QVariantMap &params);

signals:
    void selectionChanged();

private:
    void updateEngineSelection();

    QJSEngine m_engine;
    QVariantList m_selection;
};
