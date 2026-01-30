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

#include "FileOperationUtils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>

namespace FileOperationUtils {

void applyFileTimes(const QFileInfo &sourceInfo, const QString &targetPath)
{
    QFile targetFile(targetPath);
    targetFile.open(QIODevice::ReadWrite);
    targetFile.setFileTime(sourceInfo.lastModified(), QFileDevice::FileModificationTime);
    const QDateTime birthTime = sourceInfo.birthTime();
    if (birthTime.isValid()) {
        targetFile.setFileTime(birthTime, QFileDevice::FileBirthTime);
    }
    targetFile.close();
}

bool copyFolderRecursive(const QString &sourcePath, const QString &targetPath, const char *context, QString *error)
{
    const QDir sourceDir(sourcePath);
    if (!sourceDir.exists()) {
        if (error) {
            *error = QCoreApplication::translate(context, "Source not found");
        }
        return false;
    }
    if (QFileInfo::exists(targetPath)) {
        if (error) {
            *error = QCoreApplication::translate(context, "Target already exists");
        }
        return false;
    }
    if (!QDir().mkpath(targetPath)) {
        if (error) {
            *error = QCoreApplication::translate(context, "Cannot create target folder");
        }
        return false;
    }

    const QFileInfoList entries = sourceDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::NoSort);
    for (const QFileInfo &entry : entries) {
        const QString entryPath = entry.absoluteFilePath();
        const QString targetEntryPath = QDir(targetPath).filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyFolderRecursive(entryPath, targetEntryPath, context, error)) {
                return false;
            }
        } else {
            if (QFileInfo::exists(targetEntryPath)) {
                if (error) {
                    *error = QCoreApplication::translate(context, "Target already exists");
                }
                return false;
            }
            if (!QFile::copy(entryPath, targetEntryPath)) {
                if (error) {
                    *error = QCoreApplication::translate(context, "Copy failed");
                }
                return false;
            }
            applyFileTimes(entry, targetEntryPath);
        }
    }
    return true;
}

} // namespace FileOperationUtils
