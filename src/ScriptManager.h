#pragma once

#include <QObject>
#include <QVariantList>

class QFileSystemWatcher;
class QTimer;

class ScriptManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList scripts READ scripts NOTIFY scriptsChanged)

public:
    explicit ScriptManager(QObject *parent = nullptr);

    QVariantList scripts() const;

    Q_INVOKABLE void refresh();

signals:
    void scriptsChanged();

private:
    void updateScripts();
    QString resolveScriptsDir() const;

    QVariantList m_scripts;
    QString m_scriptsDir;
    QFileSystemWatcher *m_watcher;
    QTimer *m_refreshTimer;
};
