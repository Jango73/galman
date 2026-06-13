#pragma once

#include <QObject>
#include <QString>

class BackupSystem : public QObject
{
    Q_OBJECT

public:
    static constexpr const char *BackupFolderName = ".backup";

    explicit BackupSystem(QObject *parent = nullptr);

    Q_INVOKABLE QString findBackupDirectory(const QString &directoryPath) const;
    Q_INVOKABLE bool createBackupFolder(const QString &directoryPath);
    Q_INVOKABLE QString backupFile(const QString &filePath);
    Q_INVOKABLE QString restoreFile(const QString &filePath);

private:
    static int highestBackupIndex(const QString &backupDirectory,
                                  const QString &relativePath,
                                  const QString &baseName);
    static void cleanupEmptyDirectories(const QString &startDirectory);
};
