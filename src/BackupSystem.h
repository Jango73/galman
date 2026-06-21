#pragma once

#include <QObject>
#include <QString>

class BackupSystem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int maxBackupsPerFile READ maxBackupsPerFile WRITE setMaxBackupsPerFile NOTIFY maxBackupsPerFileChanged)

public:
    static constexpr const char *BackupFolderName = ".backup";

    explicit BackupSystem(QObject *parent = nullptr);

    int maxBackupsPerFile() const;
    void setMaxBackupsPerFile(int max);

    Q_INVOKABLE QString findBackupDirectory(const QString &directoryPath) const;
    Q_INVOKABLE bool hasBackupFolder(const QString &directoryPath) const;
    Q_INVOKABLE bool createBackupFolder(const QString &directoryPath);
    Q_INVOKABLE bool hasBackup(const QString &filePath) const;
    Q_INVOKABLE QString backupFile(const QString &filePath);
    Q_INVOKABLE int lastPruneCount() const;
    Q_INVOKABLE QString restoreFile(const QString &filePath);

signals:
    void maxBackupsPerFileChanged();

private:
    static int highestBackupIndex(const QString &backupDirectory,
                                  const QString &relativePath,
                                  const QString &baseName);
    static void cleanupEmptyDirectories(const QString &startDirectory);
    void pruneBackups(const QString &backupDirectory,
                      const QString &relativePath,
                      const QString &baseName);

    int m_maxBackupsPerFile = 20;
    int m_lastPruneCount = 0;
};
