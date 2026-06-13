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

#include "BackupSystem.h"
#include "PlatformUtils.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

BackupSystem::BackupSystem(QObject *parent)
    : QObject(parent)
{
}

QString BackupSystem::findBackupDirectory(const QString &directoryPath) const
{
    qInfo() << "findBackupDirectory:" << directoryPath;
    QDir dir(directoryPath);
    while (true) {
        if (QDir(dir.absolutePath()).exists(BackupFolderName)) {
            QString result = QDir(dir.absoluteFilePath(BackupFolderName)).absolutePath();
            qInfo() << "findBackupDirectory found:" << result;
            return result;
        }
        if (dir.isRoot()) {
            break;
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    qInfo() << "findBackupDirectory: no backup directory found for" << directoryPath;
    return {};
}

int BackupSystem::highestBackupIndex(const QString &backupDirectory,
                                     const QString &relativePath,
                                     const QString &baseName)
{
    QString targetDir = QDir(backupDirectory).filePath(relativePath);
    QDir dir(targetDir);
    if (!dir.exists()) {
        return -1;
    }

    QRegularExpression pattern(QStringLiteral("^%1\\.(\\d+)$").arg(QRegularExpression::escape(baseName)));
    int highest = -1;

    const QStringList entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &entry : entries) {
        QRegularExpressionMatch match = pattern.match(entry);
        if (match.hasMatch()) {
            int index = match.captured(1).toInt();
            if (index > highest) {
                highest = index;
            }
        }
    }
    return highest;
}

void BackupSystem::cleanupEmptyDirectories(const QString &startDirectory)
{
    QDir dir(startDirectory);
    while (true) {
        if (!dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
            break;
        }
        if (QFileInfo(dir.absolutePath()).fileName() == QString::fromLatin1(BackupFolderName)) {
            break;
        }
        QString parentPath = QFileInfo(dir.absolutePath()).path();
        dir.rmdir(dir.absolutePath());
        if (!dir.cdUp()) {
            break;
        }
    }
}

bool BackupSystem::createBackupFolder(const QString &directoryPath)
{
    qInfo() << "createBackupFolder:" << directoryPath;
    QDir dir(directoryPath);
    if (!dir.exists()) {
        qInfo() << "createBackupFolder: directory does not exist";
        return false;
    }
    QString backupPath = dir.absoluteFilePath(QString::fromLatin1(BackupFolderName));
    if (QFileInfo::exists(backupPath)) {
        qInfo() << "createBackupFolder: already exists:" << backupPath;
        return true;
    }
    bool ok = dir.mkdir(QString::fromLatin1(BackupFolderName));
    qInfo() << "createBackupFolder:" << (ok ? "created" : "failed") << backupPath;
    return ok;
}

QString BackupSystem::backupFile(const QString &filePath)
{
    qInfo() << "backupFile:" << filePath;
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        qInfo() << "backupFile: source does not exist or is not a file";
        return {};
    }

    QString backupDirectory = findBackupDirectory(fileInfo.absolutePath());
    if (backupDirectory.isEmpty()) {
        qInfo() << "backupFile: no backup directory found";
        return {};
    }
    qInfo() << "backupFile: backup directory:" << backupDirectory;

    QString backupRootParent = QFileInfo(backupDirectory).path();
    QDir backupRootParentDir(backupRootParent);
    QString relativePath = backupRootParentDir.relativeFilePath(fileInfo.absolutePath());
    qInfo() << "backupFile: relative path:" << relativePath;

    QString targetDir = QDir(backupDirectory).filePath(relativePath);
    QDir().mkpath(targetDir);

    QString baseName = fileInfo.fileName();
    int highest = highestBackupIndex(backupDirectory, relativePath, baseName);
    int newIndex = highest + 1;
    qInfo() << "backupFile: highest index:" << highest << "new index:" << newIndex;

    QString backupFileName = QStringLiteral("%1.%2").arg(baseName).arg(newIndex, 3, 10, QChar('0'));
    QString targetPath = QDir(targetDir).filePath(backupFileName);
    qInfo() << "backupFile: target:" << targetPath;

    if (!QFile::copy(filePath, targetPath)) {
        qInfo() << "backupFile: copy failed";
        return {};
    }
    qInfo() << "backupFile: success";
    return targetPath;
}

QString BackupSystem::restoreFile(const QString &filePath)
{
    qInfo() << "restoreFile:" << filePath;
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qInfo() << "restoreFile: source does not exist";
        return {};
    }

    QString backupDirectory = findBackupDirectory(fileInfo.absolutePath());
    if (backupDirectory.isEmpty()) {
        qInfo() << "restoreFile: no backup directory found";
        return {};
    }
    qInfo() << "restoreFile: backup directory:" << backupDirectory;

    QString backupRootParent = QFileInfo(backupDirectory).path();
    QDir backupRootParentDir(backupRootParent);
    QString relativePath = backupRootParentDir.relativeFilePath(fileInfo.absolutePath());

    QString baseName = fileInfo.fileName();
    int highest = highestBackupIndex(backupDirectory, relativePath, baseName);
    qInfo() << "restoreFile: highest index:" << highest;
    if (highest < 0) {
        qInfo() << "restoreFile: no backup found";
        return {};
    }

    QString backupFileName = QStringLiteral("%1.%2").arg(baseName).arg(highest, 3, 10, QChar('0'));
    QString backupFilePath = QDir(backupDirectory).filePath(relativePath + QDir::separator() + backupFileName);
    qInfo() << "restoreFile: backup source:" << backupFilePath;

    QString targetDir = QFileInfo(filePath).absolutePath();
    QDir().mkpath(targetDir);

    QString error;
    if (!PlatformUtils::moveToTrashOrDelete(filePath, &error)) {
        qInfo() << "restoreFile: trash failed:" << error;
        return {};
    }
    if (!QFile::rename(backupFilePath, filePath)) {
        qInfo() << "restoreFile: rename failed";
        return {};
    }

    QString backupFileDir = QFileInfo(backupFilePath).absolutePath();
    cleanupEmptyDirectories(backupFileDir);

    qInfo() << "restoreFile: success";
    return backupFilePath;
}
