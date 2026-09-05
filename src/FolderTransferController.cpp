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

#include "FolderTransferController.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QDebug>

#include "FileOperationUtils.h"
#include "PlatformUtils.h"
#include "SelectionStatisticsUtils.h"

FolderTransferController::FolderTransferController(QObject *parent)
    : QObject(parent)
{
}

FolderTransferController::~FolderTransferController()
{
    cancelCopy();
    cancelTrash();
}

bool FolderTransferController::copyInProgress() const
{
    return m_copyInProgress;
}

qreal FolderTransferController::copyProgress() const
{
    return m_copyProgress;
}

bool FolderTransferController::trashInProgress() const
{
    return m_trashInProgress;
}

qreal FolderTransferController::trashProgress() const
{
    return m_trashProgress;
}

QVariantMap FolderTransferController::copySelectedPaths(const QStringList &selectedPaths,
                                                        const QString &targetFolder,
                                                        const QString &context)
{
    qInfo() << "FolderTransferController::copySelectedPaths" << selectedPaths.size() << targetFolder;
    QVariantMap result;
    result.insert("ok", false);
    if (targetFolder.isEmpty() || !QDir(targetFolder).exists()) {
        result.insert("error", tr("Target folder not found"));
        return result;
    }

    const int total = selectedPaths.size();
    int processed = 0;
    setCopyInProgress(true);
    updateCopyProgress(0, total);

    int copied = 0;
    int failed = 0;
    QString firstError;
    const QDir folder(targetFolder);
    auto finishItem = [&]() {
        processed += 1;
        updateCopyProgress(processed, total);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    };

    for (const QString &path : selectedPaths) {
        const QFileInfo info(path);
        if (!info.exists()) {
            failed += 1;
            if (firstError.isEmpty()) {
                firstError = tr("Source not found");
            }
            finishItem();
            continue;
        }
        const QString targetPath = folder.filePath(info.fileName());
        if (QFileInfo::exists(targetPath)) {
            QString error;
            if (!PlatformUtils::moveToTrashOrDelete(targetPath, &error)) {
                failed += 1;
                if (firstError.isEmpty()) {
                    firstError = error.isEmpty() ? tr("Failed to move target to trash") : error;
                }
                finishItem();
                continue;
            }
        }
        if (info.isDir()) {
            QString error;
            if (!FileOperationUtils::copyFolderRecursive(path, targetPath, context.toUtf8().constData(), &error)) {
                failed += 1;
                if (firstError.isEmpty()) {
                    firstError = error.isEmpty() ? tr("Copy failed") : error;
                }
                finishItem();
                continue;
            }
            copied += 1;
        } else {
            if (!QFile::copy(path, targetPath)) {
                failed += 1;
                if (firstError.isEmpty()) {
                    firstError = tr("Copy failed");
                }
                finishItem();
                continue;
            }
            FileOperationUtils::applyFileTimes(info, targetPath);
            copied += 1;
        }
        finishItem();
    }

    result.insert("copied", copied);
    result.insert("failed", failed);
    if (!firstError.isEmpty()) {
        result.insert("error", firstError);
    }
    result.insert("ok", failed == 0);
    setCopyInProgress(false);
    return result;
}

void FolderTransferController::startTransferPaths(const QStringList &selectedPaths,
                                                  const QString &targetFolder,
                                                  bool moveItems,
                                                  int ghostCount,
                                                  const QString &ghostError)
{
    qInfo() << "FolderTransferController::startTransferPaths" << selectedPaths.size() << targetFolder << moveItems;
    if (m_copyInProgress) {
        return;
    }
    if (targetFolder.isEmpty() || !QDir(targetFolder).exists()) {
        emit copyFinished(targetFolderError());
        return;
    }
    if (selectedPaths.isEmpty()) {
        emit copyFinished(noSelectionError());
        return;
    }

    QList<CopyItem> items;
    items.reserve(selectedPaths.size());
    const QDir folder(targetFolder);
    for (const QString &path : selectedPaths) {
        const QFileInfo info(path);
        CopyItem item;
        item.sourcePath = path;
        item.targetPath = folder.filePath(info.fileName());
        item.isDir = info.isDir();
        items.append(item);
    }
    if (ghostCount > 0) {
        Q_UNUSED(ghostCount);
        Q_UNUSED(ghostError);
    }

    const CopyWorker::OperationMode mode = moveItems ? CopyWorker::OperationMode::Move : CopyWorker::OperationMode::Copy;
    auto *thread = new QThread(this);
    auto *worker = new CopyWorker(items, mode);
    worker->moveToThread(thread);

    m_copyThread = thread;
    m_copyWorker = worker;
    setCopyInProgress(true);
    updateCopyProgress(0, 0);

    connect(thread, &QThread::started, worker, &CopyWorker::start);
    connect(worker, &CopyWorker::progress, this, &FolderTransferController::updateCopyProgress);
    connect(worker, &CopyWorker::finished, this, [this, thread, worker](const QVariantMap &result) {
        setCopyInProgress(false);
        updateCopyProgress(0, 0);
        emit copyFinished(result);
        m_copyWorker = nullptr;
        m_copyThread = nullptr;
        thread->quit();
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void FolderTransferController::startRemovalPaths(const QStringList &selectedPaths, bool moveToTrash)
{
    qInfo() << "FolderTransferController::startRemovalPaths" << selectedPaths.size() << moveToTrash;
    if (m_trashInProgress) {
        return;
    }
    if (selectedPaths.isEmpty()) {
        emit trashFinished(nothingToDeleteError());
        return;
    }

    auto *thread = new QThread(this);
    const TrashWorker::RemovalMode mode = moveToTrash ? TrashWorker::MoveToTrash : TrashWorker::DeletePermanently;
    auto *worker = new TrashWorker(selectedPaths, mode);
    worker->moveToThread(thread);

    m_trashThread = thread;
    m_trashWorker = worker;
    setTrashInProgress(true);
    updateTrashProgress(emptyCount, selectedPaths.size());

    connect(thread, &QThread::started, worker, &TrashWorker::start);
    connect(worker, &TrashWorker::progress, this, &FolderTransferController::updateTrashProgress);
    connect(worker, &TrashWorker::finished, this, [this, thread](const QVariantMap &result) {
        setTrashInProgress(false);
        updateTrashProgress(emptyCount, emptyCount);
        emit trashFinished(result);
        m_trashWorker = nullptr;
        m_trashThread = nullptr;
        thread->quit();
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

QVariantMap FolderTransferController::requestRemovalPaths(const QStringList &selectedPaths, bool moveToTrash)
{
    qInfo() << "FolderTransferController::requestRemovalPaths" << selectedPaths.size() << moveToTrash;
    QVariantMap result;
    if (selectedPaths.isEmpty()) {
        result.insert("ok", false);
        result.insert("error", tr("Nothing to delete"));
        return result;
    }
    startRemovalPaths(selectedPaths, moveToTrash);
    result.insert("ok", true);
    result.insert("pending", true);
    return result;
}

void FolderTransferController::cancelCopy()
{
    if (!m_copyWorker) {
        return;
    }
    QMetaObject::invokeMethod(m_copyWorker, "cancel", Qt::QueuedConnection);
}

void FolderTransferController::cancelTrash()
{
    if (!m_trashWorker) {
        return;
    }
    QMetaObject::invokeMethod(m_trashWorker, "cancel", Qt::QueuedConnection);
}

int FolderTransferController::countNameConflicts(const QStringList &selectedPaths, const QString &targetFolder)
{
    return SelectionStatisticsUtils::countNameConflicts(selectedPaths, targetFolder);
}

QVariantMap FolderTransferController::targetFolderError()
{
    QVariantMap result;
    result.insert("ok", false);
    result.insert("error", tr("Target folder not found"));
    return result;
}

QVariantMap FolderTransferController::noSelectionError()
{
    QVariantMap result;
    result.insert("ok", false);
    result.insert("error", tr("No items selected"));
    return result;
}

QVariantMap FolderTransferController::nothingToDeleteError()
{
    QVariantMap result;
    result.insert("ok", false);
    result.insert("error", tr("Nothing to delete"));
    return result;
}

void FolderTransferController::setCopyInProgress(bool inProgress)
{
    if (m_copyInProgress == inProgress) {
        return;
    }
    m_copyInProgress = inProgress;
    emit copyInProgressChanged();
}

void FolderTransferController::updateCopyProgress(int completed, int total)
{
    m_copyCompleted = completed;
    m_copyTotal = total;
    const qreal nextProgress = total > 0 ? static_cast<qreal>(completed) / static_cast<qreal>(total) : zeroProgress;
    if (!qFuzzyCompare(m_copyProgress, nextProgress)) {
        m_copyProgress = nextProgress;
        emit copyProgressChanged();
    }
}

void FolderTransferController::setTrashInProgress(bool inProgress)
{
    if (m_trashInProgress == inProgress) {
        return;
    }
    m_trashInProgress = inProgress;
    emit trashInProgressChanged();
}

void FolderTransferController::updateTrashProgress(int completed, int total)
{
    m_trashCompleted = completed;
    m_trashTotal = total;
    const qreal nextProgress = total > emptyCount ? static_cast<qreal>(completed) / static_cast<qreal>(total) : zeroProgress;
    if (!qFuzzyCompare(m_trashProgress, nextProgress)) {
        m_trashProgress = nextProgress;
        emit trashProgressChanged();
    }
}
