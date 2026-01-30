
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

#include "CopyWorker.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

#include "FileOperationUtils.h"
#include "PlatformUtils.h"

/**
 * @brief Creates a copy worker for a list of copy items.
 * @param items Items describing the copy workload.
 * @param parent Parent QObject for ownership.
 */
CopyWorker::CopyWorker(const QList<CopyItem> &items, QObject *parent)
    : QObject(parent)
    , m_items(items)
{
}

/**
 * @brief Requests cancellation of the copy operation.
 */
void CopyWorker::cancel()
{
    m_cancelled.storeRelaxed(1);
}

/**
 * @brief Reports whether cancellation has been requested.
 * @return True when cancellation was requested, false otherwise.
 */
bool CopyWorker::isCancelled() const
{
    return m_cancelled.loadRelaxed() != 0;
}

/**
 * @brief Increments completed count and emits progress.
 * @param completed Reference to completed counter to increment.
 * @param total Total number of entries to process.
 */
void CopyWorker::tick(int &completed, int total)
{
    completed += 1;
    emit progress(completed, total);
}

/**
 * @brief Counts entries for a copy item, including nested items for folders.
 * @param item Copy item to inspect.
 * @param count Reference to running total count to update.
 * @param firstError First error message to set if a problem occurs.
 * @return True if counting succeeded for this item, false otherwise.
 */
bool CopyWorker::countEntries(const CopyItem &item, int &count, QString &firstError)
{
    if (isCancelled()) {
        return false;
    }
    QFileInfo info(item.sourcePath);
    if (!info.exists()) {
        if (firstError.isEmpty()) {
            firstError = tr("Source not found");
        }
        count += 1;
        return false;
    }
    count += 1;
    if (!item.isDir) {
        return true;
    }
    QDirIterator it(item.sourcePath,
                    QDir::AllEntries | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (isCancelled()) {
            return false;
        }
        it.next();
        count += 1;
    }
    return true;
}

/**
 * @brief Recursively copies a folder and its contents.
 * @param sourcePath Source folder path to copy.
 * @param targetPath Target folder path to create and fill.
 * @param completed Reference to completed counter to update.
 * @param total Total number of entries to process.
 * @param firstError First error message to set if a problem occurs.
 * @return True when the full folder copy succeeds, false otherwise.
 */
bool CopyWorker::copyDirRecursive(const QString &sourcePath,
                                  const QString &targetPath,
                                  int &completed,
                                  int total,
                                  QString &firstError)
{
    if (isCancelled()) {
        if (firstError.isEmpty()) {
            firstError = tr("Copy cancelled");
        }
        return false;
    }
    if (QFileInfo::exists(targetPath)) {
        if (firstError.isEmpty()) {
            firstError = tr("Target already exists");
        }
        tick(completed, total);
        return false;
    }
    if (!QDir().mkpath(targetPath)) {
        if (firstError.isEmpty()) {
            firstError = tr("Cannot create target folder");
        }
        tick(completed, total);
        return false;
    }
    tick(completed, total);

    QDir sourceDir(sourcePath);
    const QFileInfoList entries = sourceDir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot,
        QDir::NoSort
    );
    for (const QFileInfo &entry : entries) {
        if (isCancelled()) {
            if (firstError.isEmpty()) {
                firstError = tr("Copy cancelled");
            }
            return false;
        }
        const QString entryPath = entry.absoluteFilePath();
        const QString targetEntryPath = QDir(targetPath).filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyDirRecursive(entryPath, targetEntryPath, completed, total, firstError)) {
                return false;
            }
            continue;
        }
        if (QFileInfo::exists(targetEntryPath)) {
            if (firstError.isEmpty()) {
                firstError = tr("Target already exists");
            }
            tick(completed, total);
            return false;
        }
        if (!QFile::copy(entryPath, targetEntryPath)) {
            if (firstError.isEmpty()) {
                firstError = tr("Copy failed");
            }
            tick(completed, total);
            return false;
        }
        FileOperationUtils::applyFileTimes(entry, targetEntryPath);
        tick(completed, total);
    }
    return true;
}

/**
 * @brief Copies a single item, handling folders and files.
 * @param item Copy item to process.
 * @param completed Reference to completed counter to update.
 * @param total Total number of entries to process.
 * @param firstError First error message to set if a problem occurs.
 * @return True if the item was copied successfully, false otherwise.
 */
bool CopyWorker::copyItem(const CopyItem &item, int &completed, int total, QString &firstError)
{
    if (isCancelled()) {
        if (firstError.isEmpty()) {
            firstError = tr("Copy cancelled");
        }
        return false;
    }
    QFileInfo info(item.sourcePath);
    if (!info.exists()) {
        if (firstError.isEmpty()) {
            firstError = tr("Source not found");
        }
        tick(completed, total);
        return false;
    }
    if (QFileInfo::exists(item.targetPath)) {
        QString error;
        if (!PlatformUtils::moveToTrashOrDelete(item.targetPath, &error)) {
            if (firstError.isEmpty()) {
                firstError = error.isEmpty() ? tr("Failed to move target to trash") : error;
            }
            tick(completed, total);
            return false;
        }
    }
    if (item.isDir) {
        return copyDirRecursive(item.sourcePath, item.targetPath, completed, total, firstError);
    }
    if (!QFile::copy(item.sourcePath, item.targetPath)) {
        if (firstError.isEmpty()) {
            firstError = tr("Copy failed");
        }
        tick(completed, total);
        return false;
    }
    FileOperationUtils::applyFileTimes(info, item.targetPath);
    tick(completed, total);
    return true;
}

/**
 * @brief Executes the copy workload and emits progress and completion.
 */
void CopyWorker::start()
{
    QVariantMap result;
    result.insert("ok", false);

    int total = 0;
    QString firstError;
    for (const CopyItem &item : m_items) {
        if (isCancelled()) {
            break;
        }
        countEntries(item, total, firstError);
    }

    if (isCancelled()) {
        result.insert("error", tr("Copy cancelled"));
        result.insert("cancelled", true);
        emit finished(result);
        return;
    }

    int completed = 0;
    emit progress(0, total);

    int copied = 0;
    int failed = 0;
    for (const CopyItem &item : m_items) {
        if (isCancelled()) {
            break;
        }
        if (copyItem(item, completed, total, firstError)) {
            copied += 1;
        } else {
            failed += 1;
        }
    }

    if (isCancelled()) {
        if (firstError.isEmpty()) {
            firstError = tr("Copy cancelled");
        }
        result.insert("cancelled", true);
    }

    result.insert("copied", copied);
    result.insert("failed", failed);
    if (!firstError.isEmpty()) {
        result.insert("error", firstError);
    }
    result.insert("ok", failed == 0 && !result.value("cancelled").toBool());
    emit finished(result);
}
