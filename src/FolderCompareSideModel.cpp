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

#include "FolderCompareSideModel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLocale>
#include <QtConcurrent>
#include <QDebug>
#include <QThread>

#include "ApplicationSettings.h"
#include "CopyWorker.h"
#include "FolderCompareModel.h"
#include "FolderEntryView.h"
#include "FolderFilterSortUtils.h"
#include "ImageMetadataUtils.h"
#include "PlatformUtils.h"
#include "RowMatchUtils.h"
#include "SelectionStatisticsUtils.h"
#include "TrashWorker.h"
#include "VideoThumbnailUtils.h"

namespace {

bool compareEntryEquivalent(const FolderCompareSideModel::CompareEntry &left,
                            const FolderCompareSideModel::CompareEntry &right)
{
    return left.id == right.id && left.fileName == right.fileName && left.filePath == right.filePath
        && left.otherSidePath == right.otherSidePath && left.created == right.created
        && left.modified == right.modified && left.isDir == right.isDir && left.isImage == right.isImage
        && left.isVideo == right.isVideo && left.isGhost == right.isGhost && left.isNewer == right.isNewer
        && left.status == right.status;
}

QString previewPathForEntry(const FolderCompareSideModel::CompareEntry &entry)
{
    if (entry.isGhost && !entry.otherSidePath.isEmpty()) {
        return entry.otherSidePath;
    }
    return entry.filePath;
}

QString thumbnailRevisionForEntry(const FolderCompareSideModel::CompareEntry &entry)
{
    const QString previewPath = previewPathForEntry(entry);
    if (previewPath.isEmpty() || entry.isDir) {
        return QString();
    }
    return QStringLiteral("%1:%2").arg(QString::number(entry.modified.toMSecsSinceEpoch())).arg(previewPath);
}

FolderEntryView::EntryView viewForCompareEntry(const FolderCompareSideModel::CompareEntry &entry)
{
    FolderEntryView::EntryView view;
    view.fileName = entry.fileName;
    view.filePath = entry.filePath;
    view.isFolder = entry.isDir;
    view.isGhost = entry.isGhost;
    view.createdTime = entry.created;
    view.modifiedTime = entry.modified;
    view.isImageFlag = entry.isImage;
    view.isVideoFlag = entry.isVideo;
    if (entry.isDir || entry.isGhost || entry.filePath.isEmpty()) {
        view.byteSize = -1;
    } else {
        view.byteSize = QFileInfo(entry.filePath).size();
    }
    return view;
}

const FolderCompareSideModel::CompareEntry *findEntryById(const QVector<FolderCompareSideModel::CompareEntry> &entries,
                                                          const QString &id)
{
    for (const auto &entry : entries) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

FolderCompareSideModel::FolderCompareSideModel(FolderCompareModel *compareModel, int side, QObject *parent)
    : QAbstractListModel(parent)
    , m_compareModel(compareModel)
    , m_side(side)
    , m_transferController(this)
    , m_attributeCache(this)
{
    qInfo() << "FolderCompareSideModel::FolderCompareSideModel" << side;
    if (m_compareModel) {
        connect(m_compareModel, &FolderCompareModel::loadingChanged, this,
                [this]() { setLoading(m_compareModel->loading()); });
    }
    connect(&m_transferController, &FolderTransferController::copyInProgressChanged, this,
            [this]() { emit copyInProgressChanged(); });
    connect(&m_transferController, &FolderTransferController::copyProgressChanged, this,
            [this]() { emit copyProgressChanged(); });
    connect(&m_transferController, &FolderTransferController::trashInProgressChanged, this,
            [this]() { emit trashInProgressChanged(); });
    connect(&m_transferController, &FolderTransferController::trashProgressChanged, this,
            [this]() { emit trashProgressChanged(); });
}

QString FolderCompareSideModel::rootPath() const
{
    return m_rootPath;
}

void FolderCompareSideModel::setRootPath(const QString &path)
{
    qInfo() << "FolderCompareSideModel::setRootPath" << path;
    if (path.isEmpty() || path == m_rootPath) {
        return;
    }
    m_rootPath = path;
    emit rootPathChanged();
    if (!m_settingsKey.isEmpty()) {
        ApplicationSettings settings;
        settings.setValue(m_settingsKey, m_rootPath);
    }
    if (m_compareModel) {
        m_compareModel->setSidePath(static_cast<FolderCompareModel::Side>(m_side), m_rootPath);
    }
    refresh();
    clearSelection();
}

QString FolderCompareSideModel::settingsKey() const
{
    return m_settingsKey;
}

void FolderCompareSideModel::setSettingsKey(const QString &key)
{
    if (key == m_settingsKey) {
        return;
    }
    m_settingsKey = key;
    emit settingsKeyChanged();
    if (m_settingsKey.isEmpty()) {
        return;
    }
    ApplicationSettings settings;
    const QString storedPath = settings.value(m_settingsKey).toString();
    if (!storedPath.isEmpty() && QDir(storedPath).exists() && storedPath != m_rootPath) {
        setRootPath(storedPath);
    }
}

QString FolderCompareSideModel::nameFilter() const
{
    return m_filterSettings.nameFilter();
}

void FolderCompareSideModel::setNameFilter(const QString &filter)
{
    qInfo() << "FolderCompareSideModel::setNameFilter" << filter;
    if (filter == m_filterSettings.nameFilter()) {
        return;
    }
    m_filterSettings.setNameFilter(filter);
    emit nameFilterChanged();
    rebuildEntries();
}

FolderCompareSideModel::SortKey FolderCompareSideModel::sortKey() const
{
    return static_cast<SortKey>(m_filterSettings.sortKeyValue());
}

void FolderCompareSideModel::setSortKey(FolderCompareSideModel::SortKey key)
{
    if (static_cast<int>(key) == m_filterSettings.sortKeyValue()) {
        return;
    }
    m_filterSettings.setSortKeyValue(static_cast<int>(key));
    emit sortKeyChanged();
    rebuildEntries();
}

Qt::SortOrder FolderCompareSideModel::sortOrder() const
{
    return m_filterSettings.sortOrder();
}

void FolderCompareSideModel::setSortOrder(Qt::SortOrder order)
{
    if (order == m_filterSettings.sortOrder()) {
        return;
    }
    m_filterSettings.setSortOrder(order);
    emit sortOrderChanged();
    rebuildEntries();
}

bool FolderCompareSideModel::showDirsFirst() const
{
    return m_filterSettings.showFoldersFirst();
}

bool FolderCompareSideModel::hideJunkFiles() const
{
    return m_filterSettings.hideJunkFiles();
}

void FolderCompareSideModel::setHideJunkFiles(bool enabled)
{
    if (enabled == m_filterSettings.hideJunkFiles()) {
        return;
    }
    m_filterSettings.setHideJunkFiles(enabled);
    emit hideJunkFilesChanged();
    rebuildEntries();
}

void FolderCompareSideModel::setShowDirsFirst(bool enabled)
{
    if (enabled == m_filterSettings.showFoldersFirst()) {
        return;
    }
    m_filterSettings.setShowFoldersFirst(enabled);
    emit showDirsFirstChanged();
    rebuildEntries();
}

qint64 FolderCompareSideModel::minimumByteSize() const
{
    return m_filterSettings.minimumByteSize();
}

void FolderCompareSideModel::setMinimumByteSize(qint64 value)
{
    if (FolderFilterSettings::normalizedByteSize(value) == m_filterSettings.minimumByteSize()) {
        return;
    }
    m_filterSettings.setMinimumByteSize(value);
    emit minimumByteSizeChanged();
    rebuildEntries();
}

qint64 FolderCompareSideModel::maximumByteSize() const
{
    return m_filterSettings.maximumByteSize();
}

void FolderCompareSideModel::setMaximumByteSize(qint64 value)
{
    if (FolderFilterSettings::normalizedByteSize(value) == m_filterSettings.maximumByteSize()) {
        return;
    }
    m_filterSettings.setMaximumByteSize(value);
    emit maximumByteSizeChanged();
    rebuildEntries();
}

int FolderCompareSideModel::minimumImageWidth() const
{
    return m_filterSettings.minimumImageWidth();
}

void FolderCompareSideModel::setMinimumImageWidth(int value)
{
    if (FolderFilterSettings::normalizedDimension(value) == m_filterSettings.minimumImageWidth()) {
        return;
    }
    m_filterSettings.setMinimumImageWidth(value);
    emit minimumImageWidthChanged();
    rebuildEntries();
}

int FolderCompareSideModel::maximumImageWidth() const
{
    return m_filterSettings.maximumImageWidth();
}

void FolderCompareSideModel::setMaximumImageWidth(int value)
{
    if (FolderFilterSettings::normalizedDimension(value) == m_filterSettings.maximumImageWidth()) {
        return;
    }
    m_filterSettings.setMaximumImageWidth(value);
    emit maximumImageWidthChanged();
    rebuildEntries();
}

int FolderCompareSideModel::minimumImageHeight() const
{
    return m_filterSettings.minimumImageHeight();
}

void FolderCompareSideModel::setMinimumImageHeight(int value)
{
    if (FolderFilterSettings::normalizedDimension(value) == m_filterSettings.minimumImageHeight()) {
        return;
    }
    m_filterSettings.setMinimumImageHeight(value);
    emit minimumImageHeightChanged();
    rebuildEntries();
}

int FolderCompareSideModel::maximumImageHeight() const
{
    return m_filterSettings.maximumImageHeight();
}

void FolderCompareSideModel::setMaximumImageHeight(int value)
{
    if (FolderFilterSettings::normalizedDimension(value) == m_filterSettings.maximumImageHeight()) {
        return;
    }
    m_filterSettings.setMaximumImageHeight(value);
    emit maximumImageHeightChanged();
    rebuildEntries();
}

bool FolderCompareSideModel::loading() const
{
    return m_loading;
}

void FolderCompareSideModel::setHideIdentical(bool hide)
{
    if (m_hideIdentical == hide) {
        return;
    }
    m_hideIdentical = hide;
    rebuildEntries();
}

QStringList FolderCompareSideModel::selectedPaths() const
{
    return m_selectedPaths;
}

bool FolderCompareSideModel::selectedIsImage() const
{
    return m_selectedIsImage;
}

bool FolderCompareSideModel::selectedIsVideo() const
{
    return m_selectedIsVideo;
}

int FolderCompareSideModel::selectedFileCount() const
{
    return m_selectedFileCount;
}

qint64 FolderCompareSideModel::selectedTotalBytes() const
{
    return m_selectedTotalBytes;
}

bool FolderCompareSideModel::copyInProgress() const
{
    return m_transferController.copyInProgress();
}

qreal FolderCompareSideModel::copyProgress() const
{
    return m_transferController.copyProgress();
}

bool FolderCompareSideModel::trashInProgress() const
{
    return m_transferController.trashInProgress();
}

qreal FolderCompareSideModel::trashProgress() const
{
    return m_transferController.trashProgress();
}

int FolderCompareSideModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant FolderCompareSideModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const CompareEntry &entry = m_entries.at(index.row());
    switch (role) {
    case FileNameRole:
        return entry.fileName;
    case FilePathRole:
        return entry.filePath;
    case OtherSidePathRole:
        return entry.otherSidePath;
    case IsDirRole:
        return entry.isDir;
    case IsImageRole:
        return entry.isImage;
    case IsVideoRole:
        return entry.isVideo;
    case ThumbnailPathRole: {
        const QString previewPath = previewPathForEntry(entry);
        if (previewPath.isEmpty()) {
            return QString();
        }
        if (entry.isImage) {
            return previewPath;
        }
        if (entry.isVideo) {
            return m_attributeCache.videoThumbnails().value(previewPath);
        }
        return QString();
    }
    case ThumbnailRevisionRole:
        return thumbnailRevisionForEntry(entry);
    case SuffixRole:
        return FolderEntryView::suffixForFileName(entry.fileName);
    case CreatedRole:
        return entry.created;
    case ModifiedRole:
        return entry.modified;
    case SelectedRole:
        return m_selectionManager.isSelected(entry.id);
    case CompareStatusRole:
        return static_cast<int>(entry.status);
    case GhostRole:
        return entry.isGhost;
    case IsNewerRole:
        return entry.isNewer;
    default:
        return {};
    }
}

QHash<int, QByteArray> FolderCompareSideModel::roleNames() const
{
    return {{FileNameRole, "fileName"},
            {FilePathRole, "filePath"},
            {OtherSidePathRole, "otherSidePath"},
            {IsDirRole, "isDir"},
            {IsImageRole, "isImage"},
            {IsVideoRole, "isVideo"},
            {ThumbnailPathRole, "thumbnailPath"},
            {ThumbnailRevisionRole, "thumbnailRevision"},
            {SuffixRole, "suffix"},
            {CreatedRole, "created"},
            {ModifiedRole, "modified"},
            {SelectedRole, "selected"},
            {CompareStatusRole, "compareStatus"},
            {GhostRole, "isGhost"},
            {IsNewerRole, "isNewer"}};
}

void FolderCompareSideModel::refresh()
{
    qInfo() << "FolderCompareSideModel::refresh";
    if (m_compareModel) {
        m_compareModel->requestRefresh();
    }
}

const FolderCompareSideModel::CompareEntry *FolderCompareSideModel::entryForRow(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return nullptr;
    }
    return &m_entries.at(row);
}

FolderCompareSideModel::CompareEntry *FolderCompareSideModel::entryForRow(int row)
{
    if (row < 0 || row >= m_entries.size()) {
        return nullptr;
    }
    return &m_entries[row];
}

QString FolderCompareSideModel::keyForRow(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return QString();
    }
    return entry->id;
}

void FolderCompareSideModel::activate(int row)
{
    qInfo() << "FolderCompareSideModel::activate" << row;
    const CompareEntry *entry = entryForRow(row);
    if (!entry || entry->isGhost) {
        return;
    }
    if (entry->isDir) {
        emit folderActivated(entry->filePath);
    } else {
        emit fileActivated(entry->filePath);
    }
}

void FolderCompareSideModel::select(int row, bool multi)
{
    const QString key = keyForRow(row);
    if (key.isEmpty()) {
        return;
    }
    bool changed = multi ? m_selectionManager.toggleKey(key) : m_selectionManager.selectSingle(key);
    if (changed) {
        notifySelectionChanged();
    }
}

bool FolderCompareSideModel::isSelected(int row) const
{
    return m_selectionManager.isSelected(keyForRow(row));
}

bool FolderCompareSideModel::isDir(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    return entry && entry->isDir;
}

QString FolderCompareSideModel::pathForRow(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    return entry ? entry->filePath : QString();
}

int FolderCompareSideModel::rowForPrefix(const QString &prefix, int startRow) const
{
    return RowMatchUtils::rowForPrefix(
        prefix, startRow, m_entries.size(), [this](int row) { return m_entries.at(row).fileName; });
}

QString FolderCompareSideModel::modifiedForRow(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return {};
    }
    return QLocale().toString(entry->modified, QLocale::ShortFormat);
}

int FolderCompareSideModel::copyNameConflictCount(const QString &targetFolder) const
{
    return FolderTransferController::countNameConflicts(m_selectedPaths, targetFolder);
}

void FolderCompareSideModel::clearSelection()
{
    if (m_selectionManager.clearSelection()) {
        notifySelectionChanged();
    }
}

void FolderCompareSideModel::goUp()
{
    qInfo() << "FolderCompareSideModel::goUp" << m_rootPath;
    if (m_rootPath.isEmpty()) {
        return;
    }
    QDir folder(m_rootPath);
    if (folder.isRoot()) {
        return;
    }
    m_pendingSelectionId = m_rootPath;
    folder.cdUp();
    setRootPath(folder.absolutePath());
}

void FolderCompareSideModel::setSelection(const QVariantList &rows, bool additive)
{
    if (m_selectionManager.setFromRowsGeneric(
            rows, additive, m_entries.size(), [this](int row) { return keyForRow(row); })) {
        notifySelectionChanged();
    }
}

void FolderCompareSideModel::setSelectionRange(int start, int end, bool additive)
{
    if (m_entries.isEmpty()) {
        return;
    }
    const int lower = std::max(0, std::min(start, end));
    const int upper = std::min(std::max(start, end), static_cast<int>(m_entries.size()) - 1);
    QVariantList rows;
    rows.reserve(upper - lower + 1);
    for (int row = lower; row <= upper; ++row) {
        rows.append(row);
    }
    setSelection(rows, additive);
}

bool FolderCompareSideModel::allSelected() const
{
    return m_selectionManager.allSelected(m_entries.size());
}

QVariantList FolderCompareSideModel::selectedRows() const
{
    return m_selectionManager.selectedRows(m_entries.size(),
                                           [this](int row) { return keyForRow(row); });
}

bool FolderCompareSideModel::isImage(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    return entry && entry->isImage && !entry->isDir;
}

bool FolderCompareSideModel::isVideo(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    return entry && entry->isVideo && !entry->isDir && !entry->isGhost;
}

bool FolderCompareSideModel::isGhost(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    return entry && entry->isGhost;
}

int FolderCompareSideModel::selectedCompareStatus() const
{
    if (m_selectionManager.selectedKeys().isEmpty()) {
        return StatusNone;
    }
    const CompareEntry *entry = findEntryById(m_entries, m_selectionManager.selectedKeys().first());
    return entry ? entry->status : StatusNone;
}

bool FolderCompareSideModel::selectedIsGhost() const
{
    if (m_selectionManager.selectedKeys().isEmpty()) {
        return false;
    }
    const CompareEntry *entry = findEntryById(m_entries, m_selectionManager.selectedKeys().first());
    return entry && entry->isGhost;
}

bool FolderCompareSideModel::selectedIsNewer() const
{
    if (m_selectionManager.selectedKeys().isEmpty()) {
        return false;
    }
    const CompareEntry *entry = findEntryById(m_entries, m_selectionManager.selectedKeys().first());
    return entry && entry->isNewer;
}

QStringList FolderCompareSideModel::validSelectedPaths() const
{
    QStringList paths;
    for (const QString &id : m_selectionManager.selectedKeys()) {
        const CompareEntry *entry = findEntryById(m_entries, id);
        if (!entry || entry->isGhost || entry->filePath.isEmpty()) {
            continue;
        }
        paths.append(entry->filePath);
    }
    return paths;
}

QVariantMap FolderCompareSideModel::selectionStats() const
{
    QVariantMap result;
    const auto stats = SelectionStatisticsUtils::computeStatistics(validSelectedPaths());
    result.insert("dirs", stats.folderCount);
    result.insert("files", stats.fileCount);
    return result;
}

QVariantMap FolderCompareSideModel::copySelectedTo(const QString &targetFolder)
{
    qInfo() << "FolderCompareSideModel::copySelectedTo" << targetFolder;
    if (targetFolder.isEmpty() || !QDir(targetFolder).exists()) {
        return FolderTransferController::targetFolderError();
    }
    int ghostFailed = 0;
    QString ghostError;
    QStringList validPaths;
    for (const QString &id : m_selectionManager.selectedKeys()) {
        const CompareEntry *entry = findEntryById(m_entries, id);
        if (!entry || entry->isGhost) {
            ghostFailed += 1;
            if (ghostError.isEmpty()) {
                ghostError = tr("Cannot copy ghost items");
            }
            continue;
        }
        if (entry->filePath.isEmpty() || !QFileInfo::exists(entry->filePath)) {
            ghostFailed += 1;
            if (ghostError.isEmpty()) {
                ghostError = tr("Source not found");
            }
            continue;
        }
        validPaths.append(entry->filePath);
    }
    QVariantMap result =
        m_transferController.copySelectedPaths(validPaths, targetFolder, "FolderCompareSideModel");
    if (ghostFailed > 0) {
        result.insert("failed", result.value("failed").toInt() + ghostFailed);
        result.insert("ok", false);
        if (!ghostError.isEmpty() && !result.contains("error")) {
            result.insert("error", ghostError);
        }
    }
    return result;
}

void FolderCompareSideModel::startTransferSelectedTo(const QString &targetFolder, bool moveItems)
{
    qInfo() << "FolderCompareSideModel::startTransferSelectedTo" << targetFolder << moveItems;
    if (m_transferController.copyInProgress()) {
        return;
    }
    if (targetFolder.isEmpty() || !QDir(targetFolder).exists()) {
        emit copyFinished(FolderTransferController::targetFolderError());
        return;
    }
    if (m_selectionManager.selectedKeys().isEmpty()) {
        emit copyFinished(FolderTransferController::noSelectionError());
        return;
    }
    m_copyExtraFailed = 0;
    m_copyExtraError.clear();
    QList<CopyItem> items;
    const QDir folder(targetFolder);
    const QString ghostError = moveItems ? tr("Cannot move ghost items") : tr("Cannot copy ghost items");
    for (const QString &id : m_selectionManager.selectedKeys()) {
        const CompareEntry *entry = findEntryById(m_entries, id);
        if (!entry || entry->isGhost) {
            m_copyExtraFailed += 1;
            if (m_copyExtraError.isEmpty()) {
                m_copyExtraError = ghostError;
            }
            continue;
        }
        if (entry->filePath.isEmpty() || !QFileInfo::exists(entry->filePath)) {
            m_copyExtraFailed += 1;
            if (m_copyExtraError.isEmpty()) {
                m_copyExtraError = tr("Source not found");
            }
            continue;
        }
        CopyItem item;
        item.sourcePath = entry->filePath;
        item.targetPath = folder.filePath(QFileInfo(entry->filePath).fileName());
        item.isDir = entry->isDir;
        items.append(item);
    }
    if (items.isEmpty()) {
        QVariantMap result;
        result.insert("ok", false);
        result.insert("failed", m_copyExtraFailed);
        result.insert(moveItems ? "moved" : "copied", 0);
        if (!m_copyExtraError.isEmpty()) {
            result.insert("error", m_copyExtraError);
        }
        emit copyFinished(result);
        m_copyExtraFailed = 0;
        m_copyExtraError.clear();
        return;
    }
    Q_UNUSED(items);
    QStringList validPaths;
    validPaths.reserve(items.size());
    for (const CopyItem &item : items) {
        validPaths.append(item.sourcePath);
    }
    auto *guard = new QObject(this);
    connect(&m_transferController, &FolderTransferController::copyFinished, guard,
            [this, guard](QVariantMap result) {
                guard->deleteLater();
                result.insert("failed", result.value("failed").toInt() + m_copyExtraFailed);
                if (!m_copyExtraError.isEmpty() && !result.contains("error")) {
                    result.insert("error", m_copyExtraError);
                }
                if (result.value("failed").toInt() > 0 || result.value("cancelled").toBool()) {
                    result.insert("ok", false);
                }
                m_copyExtraFailed = 0;
                m_copyExtraError.clear();
                emit copyFinished(result);
            });
    m_transferController.startTransferPaths(validPaths, targetFolder, moveItems, 0, {});
}

void FolderCompareSideModel::startCopySelectedTo(const QString &targetFolder)
{
    startTransferSelectedTo(targetFolder, false);
}

void FolderCompareSideModel::startMoveSelectedTo(const QString &targetFolder)
{
    startTransferSelectedTo(targetFolder, true);
}

void FolderCompareSideModel::cancelCopy()
{
    m_transferController.cancelCopy();
}

QVariantMap FolderCompareSideModel::moveSelectedToTrash()
{
    qInfo() << "FolderCompareSideModel::moveSelectedToTrash";
    return requestRemoval(true);
}

void FolderCompareSideModel::startMoveSelectedToTrash()
{
    startRemoval(true);
}

QVariantMap FolderCompareSideModel::deleteSelectedPermanently()
{
    return requestRemoval(false);
}

void FolderCompareSideModel::startDeleteSelectedPermanently()
{
    startRemoval(false);
}

QVariantMap FolderCompareSideModel::requestRemoval(bool moveToTrash)
{
    if (m_selectionManager.selectedKeys().isEmpty()) {
        return FolderTransferController::nothingToDeleteError();
    }
    startRemoval(moveToTrash);
    QVariantMap result;
    result.insert("ok", true);
    result.insert("pending", true);
    return result;
}

void FolderCompareSideModel::startRemoval(bool moveToTrash)
{
    qInfo() << "FolderCompareSideModel::startRemoval" << moveToTrash;
    if (m_transferController.trashInProgress()) {
        return;
    }
    QStringList paths = validSelectedPaths();
    int preFailed = 0;
    QString preError;
    if (paths.size() != m_selectionManager.selectedKeys().size()) {
        preFailed = m_selectionManager.selectedKeys().size() - paths.size();
        preError = moveToTrash ? tr("Cannot trash ghost items") : tr("Cannot delete ghost items");
    }
    if (paths.isEmpty()) {
        QVariantMap result;
        result.insert("ok", false);
        result.insert("failed", preFailed);
        if (!preError.isEmpty()) {
            result.insert("error", preError);
        }
        emit trashFinished(result);
        clearSelection();
        return;
    }
    auto *guard = new QObject(this);
    connect(&m_transferController, &FolderTransferController::trashFinished, guard,
            [this, guard, preFailed, preError](QVariantMap result) {
                guard->deleteLater();
                result.insert("failed", result.value("failed").toInt() + preFailed);
                if (!preError.isEmpty() && !result.contains("error")) {
                    result.insert("error", preError);
                }
                if (result.value("failed").toInt() > 0 || result.value("cancelled").toBool()) {
                    result.insert("ok", false);
                }
                emit trashFinished(result);
                const int moved = result.value("moved").toInt();
                const QStringList touched = result.value("touchedPaths").toStringList();
                if (moved > 0 && m_compareModel) {
                    if (m_side == FolderCompareModel::Left) {
                        m_compareModel->refreshFiles(touched, {});
                    } else {
                        m_compareModel->refreshFiles({}, touched);
                    }
                }
                clearSelection();
            });
    m_transferController.startRemovalPaths(paths, moveToTrash);
}

void FolderCompareSideModel::cancelTrash()
{
    m_transferController.cancelTrash();
}

QVariantMap FolderCompareSideModel::renamePath(const QString &path, const QString &newName)
{
    qInfo() << "FolderCompareSideModel::renamePath" << path << newName;
    QVariantMap result;
    result.insert("ok", false);
    if (path.isEmpty()) {
        result.insert("error", tr("Source not found"));
        return result;
    }
    for (const CompareEntry &entry : m_entries) {
        if (entry.filePath == path && entry.isGhost) {
            result.insert("error", tr("Cannot rename ghost items"));
            return result;
        }
    }
    QString targetPath;
    QString error;
    if (!PlatformUtils::renamePath(path, newName, &targetPath, &error)) {
        result.insert("error", error.isEmpty() ? tr("Rename failed") : error);
        return result;
    }
    QStringList keys = m_selectionManager.selectedKeys();
    bool selectionChanged = false;
    for (const CompareEntry &entry : m_entries) {
        if (entry.filePath == path && keys.contains(entry.id)) {
            keys.removeAll(entry.id);
            selectionChanged = true;
        }
    }
    if (selectionChanged) {
        m_selectionManager.setSelectedKeys(keys);
        notifySelectionChanged();
    }
    if (m_compareModel) {
        if (m_side == FolderCompareModel::Left) {
            m_compareModel->refreshFiles({path, targetPath}, {});
        } else {
            m_compareModel->refreshFiles({}, {path, targetPath});
        }
    }
    result.insert("ok", true);
    result.insert("newPath", targetPath);
    return result;
}

bool FolderCompareSideModel::hasGhostOnOtherSide(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry || !entry->isDir || entry->isGhost || !m_compareModel) {
        return false;
    }
    auto *otherModel = (m_side == FolderCompareModel::Left)
        ? qobject_cast<FolderCompareSideModel *>(m_compareModel->rightModel())
        : qobject_cast<FolderCompareSideModel *>(m_compareModel->leftModel());
    if (!otherModel) {
        return false;
    }
    for (const CompareEntry &otherEntry : otherModel->m_entries) {
        if (otherEntry.isGhost && otherEntry.isDir && otherEntry.fileName == entry->fileName) {
            return true;
        }
    }
    return false;
}

bool FolderCompareSideModel::createFolder(const QString &parentPath, const QString &folderName)
{
    qInfo() << "FolderCompareSideModel::createFolder" << parentPath << folderName;
    if (parentPath.isEmpty() || folderName.isEmpty()) {
        return false;
    }
    QDir folder(parentPath);
    return folder.exists() && folder.mkdir(folderName);
}

void FolderCompareSideModel::setLoading(bool loading)
{
    if (loading == m_loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

void FolderCompareSideModel::setBaseEntries(const QVector<CompareEntry> &entries)
{
    m_baseEntries = entries;
    pruneCaches(m_baseEntries);
    rebuildEntries();
}

void FolderCompareSideModel::updateBaseEntriesPartial(const QVector<CompareEntry> &entries,
                                                      const QSet<QString> &affectedNames,
                                                      const QSet<QString> &affectedPaths)
{
    if (affectedNames.isEmpty() && affectedPaths.isEmpty()) {
        return;
    }
    QVector<CompareEntry> nextBase;
    nextBase.reserve(m_baseEntries.size() + entries.size());
    for (const CompareEntry &entry : m_baseEntries) {
        const bool affected = (!entry.filePath.isEmpty() && affectedPaths.contains(entry.filePath))
            || (!entry.fileName.isEmpty() && affectedNames.contains(entry.fileName));
        if (!affected) {
            nextBase.append(entry);
        }
    }
    for (const CompareEntry &entry : entries) {
        const bool affected = (!entry.filePath.isEmpty() && affectedPaths.contains(entry.filePath))
            || (!entry.fileName.isEmpty() && affectedNames.contains(entry.fileName));
        if (affected) {
            nextBase.append(entry);
        }
    }
    m_baseEntries = std::move(nextBase);
    pruneCaches(m_baseEntries);
    QVector<CompareEntry> filtered = m_baseEntries;
    applyFilterAndSort(filtered);
    applyEntriesIncremental(filtered);
    requestVideoThumbnailRefresh();
}

void FolderCompareSideModel::rebuildEntries()
{
    if (imageSizeFiltersActive()) {
        requestImageSizeRefresh();
    }
    if (signatureSortActive()) {
        requestSignatureHashRefresh();
    }
    requestVideoThumbnailRefresh();
    QVector<CompareEntry> filtered = m_baseEntries;
    applyFilterAndSort(filtered);
    applyEntriesIncremental(filtered);
}

void FolderCompareSideModel::applyEntriesIncremental(const QVector<CompareEntry> &entries)
{
    const QVector<int> dataRoles = {FileNameRole,  FilePathRole, OtherSidePathRole, IsDirRole,
                                    IsImageRole,   IsVideoRole,  ThumbnailPathRole,  SuffixRole,
                                    CreatedRole,   ModifiedRole, CompareStatusRole,  GhostRole,
                                    IsNewerRole,   ThumbnailRevisionRole};
    int position = 0;
    while (position < entries.size()) {
        const QString &nextId = entries.at(position).id;
        if (position < m_entries.size() && m_entries.at(position).id == nextId) {
            m_entries[position] = entries.at(position);
            emit dataChanged(index(position, 0), index(position, 0), dataRoles);
            position += 1;
            continue;
        }
        int existing = -1;
        for (int search = position + 1; search < m_entries.size(); ++search) {
            if (m_entries.at(search).id == nextId) {
                existing = search;
                break;
            }
        }
        if (existing >= 0) {
            beginMoveRows(QModelIndex(), existing, existing, QModelIndex(), position);
            const CompareEntry moved = m_entries.takeAt(existing);
            m_entries.insert(position, moved);
            endMoveRows();
            m_entries[position] = entries.at(position);
            emit dataChanged(index(position, 0), index(position, 0), dataRoles);
            position += 1;
            continue;
        }
        beginInsertRows(QModelIndex(), position, position);
        m_entries.insert(position, entries.at(position));
        endInsertRows();
        position += 1;
    }
    if (m_entries.size() > entries.size()) {
        beginRemoveRows(QModelIndex(), entries.size(), m_entries.size() - 1);
        m_entries.remove(entries.size(), m_entries.size() - entries.size());
        endRemoveRows();
    }
    QStringList nextSelection;
    if (!m_pendingSelectionId.isEmpty()) {
        for (const CompareEntry &entry : entries) {
            if (entry.id == m_pendingSelectionId) {
                nextSelection.append(m_pendingSelectionId);
                break;
            }
        }
        m_pendingSelectionId.clear();
    } else {
        for (const QString &id : m_selectionManager.selectedKeys()) {
            if (findEntryById(entries, id)) {
                nextSelection.append(id);
            }
        }
    }
    if (nextSelection != m_selectionManager.selectedKeys()) {
        m_selectionManager.setSelectedKeys(nextSelection);
    }
    notifySelectionChanged();
}

bool FolderCompareSideModel::byteSizeFiltersActive() const
{
    return m_filterSettings.byteSizeFiltersActive();
}

bool FolderCompareSideModel::imageSizeFiltersActive() const
{
    return m_filterSettings.imageSizeFiltersActive();
}

bool FolderCompareSideModel::signatureSortActive() const
{
    return m_filterSettings.sortKeyValue() == FolderFilterSettings::SortBySignature;
}

void FolderCompareSideModel::pruneCaches(const QVector<CompareEntry> &entries)
{
    QSet<QString> currentPaths;
    currentPaths.reserve(entries.size() * 2);
    for (const CompareEntry &entry : entries) {
        if (!entry.filePath.isEmpty()) {
            currentPaths.insert(entry.filePath);
        }
        if (!entry.otherSidePath.isEmpty()) {
            currentPaths.insert(entry.otherSidePath);
        }
    }
    m_attributeCache.pruneCaches(currentPaths);
}

void FolderCompareSideModel::requestImageSizeRefresh()
{
    if (m_attributeCache.imageSizeLoading() || !imageSizeFiltersActive()) {
        return;
    }
    QStringList pending;
    for (const CompareEntry &entry : m_baseEntries) {
        if (entry.isDir || !entry.isImage || entry.filePath.isEmpty()
            || m_attributeCache.imageSizeAttempted(entry.filePath)) {
            continue;
        }
        pending.append(entry.filePath);
    }
    if (pending.isEmpty()) {
        return;
    }
    m_attributeCache.setImageSizeLoading(true);
    m_attributeCache.setImageSizeGeneration(m_attributeCache.imageSizeGeneration() + 1);
    const int token = m_attributeCache.imageSizeGeneration();
    auto future = QtConcurrent::run([pending]() { return ImageMetadataUtils::readImageSizes(pending); });
    auto *watcher = new QFutureWatcher<ImageMetadataUtils::ImageSizeBatchResult>(this);
    connect(watcher, &QFutureWatcher<ImageMetadataUtils::ImageSizeBatchResult>::finished, this,
            [this, watcher, token]() {
                const auto result = watcher->result();
                watcher->deleteLater();
                if (token != m_attributeCache.imageSizeGeneration()) {
                    return;
                }
                for (auto iterator = result.sizes.constBegin(); iterator != result.sizes.constEnd();
                     ++iterator) {
                    m_attributeCache.setImageSize(iterator.key(), iterator.value());
                }
                for (const QString &path : result.attempted) {
                    m_attributeCache.markImageSizeAttempted(path);
                }
                m_attributeCache.setImageSizeLoading(false);
                rebuildEntries();
            });
    watcher->setFuture(future);
}

void FolderCompareSideModel::requestSignatureHashRefresh()
{
    if (m_attributeCache.signatureLoading() || !signatureSortActive()) {
        return;
    }
    QStringList pending;
    for (const CompareEntry &entry : m_baseEntries) {
        if (entry.isDir || !entry.isImage || entry.filePath.isEmpty()
            || m_attributeCache.signatureAttempted(entry.filePath)) {
            continue;
        }
        pending.append(entry.filePath);
    }
    if (pending.isEmpty()) {
        return;
    }
    m_attributeCache.setSignatureLoading(true);
    m_attributeCache.setSignatureGeneration(m_attributeCache.signatureGeneration() + 1);
    const int token = m_attributeCache.signatureGeneration();
    auto future = QtConcurrent::run([pending]() {
        return ImageMetadataUtils::readSignatureHashes(pending, FolderFilterSettings::signatureDimension);
    });
    auto *watcher = new QFutureWatcher<ImageMetadataUtils::SignatureHashBatchResult>(this);
    connect(watcher, &QFutureWatcher<ImageMetadataUtils::SignatureHashBatchResult>::finished, this,
            [this, watcher, token]() {
                const auto result = watcher->result();
                watcher->deleteLater();
                if (token != m_attributeCache.signatureGeneration()) {
                    return;
                }
                for (auto iterator = result.hashes.constBegin(); iterator != result.hashes.constEnd();
                     ++iterator) {
                    m_attributeCache.setSignatureHash(iterator.key(), iterator.value());
                }
                for (const QString &path : result.attempted) {
                    m_attributeCache.markSignatureAttempted(path);
                }
                m_attributeCache.setSignatureLoading(false);
                rebuildEntries();
            });
    watcher->setFuture(future);
}

void FolderCompareSideModel::requestVideoThumbnailRefresh()
{
    if (m_attributeCache.videoThumbnailLoading()) {
        return;
    }
    QStringList pending;
    for (const CompareEntry &entry : m_baseEntries) {
        if (entry.isDir || !entry.isVideo) {
            continue;
        }
        const QString path = previewPathForEntry(entry);
        if (path.isEmpty() || m_attributeCache.videoThumbnailAttempted(path)) {
            continue;
        }
        const QString expected = VideoThumbnailUtils::thumbnailPathForSource(path);
        if (m_attributeCache.videoThumbnails().contains(path)) {
            const QString cached = m_attributeCache.videoThumbnails().value(path);
            if (cached == expected && QFileInfo::exists(cached)) {
                continue;
            }
            if (!cached.isEmpty()) {
                QFile::remove(cached);
            }
        }
        pending.append(path);
    }
    if (pending.isEmpty()) {
        return;
    }
    m_attributeCache.setVideoThumbnailLoading(true);
    m_attributeCache.setVideoThumbnailGeneration(m_attributeCache.videoThumbnailGeneration() + 1);
    const int token = m_attributeCache.videoThumbnailGeneration();
    auto future = QtConcurrent::run([pending]() {
        QHash<QString, QString> thumbnails;
        QStringList attempted;
        for (const QString &path : pending) {
            attempted.append(path);
            QString thumbnailPath;
            if (VideoThumbnailUtils::generateThumbnail(path, &thumbnailPath)) {
                thumbnails.insert(path, thumbnailPath);
            }
        }
        return qMakePair(thumbnails, attempted);
    });
    auto *watcher = new QFutureWatcher<QPair<QHash<QString, QString>, QStringList>>(this);
    connect(watcher, &QFutureWatcher<QPair<QHash<QString, QString>, QStringList>>::finished, this,
            [this, watcher, token]() {
                const auto result = watcher->result();
                watcher->deleteLater();
                if (token != m_attributeCache.videoThumbnailGeneration()) {
                    return;
                }
                for (auto iterator = result.first.constBegin(); iterator != result.first.constEnd();
                     ++iterator) {
                    m_attributeCache.setVideoThumbnail(iterator.key(), iterator.value());
                }
                for (const QString &path : result.second) {
                    m_attributeCache.markVideoThumbnailAttempted(path);
                }
                m_attributeCache.setVideoThumbnailLoading(false);
                if (!m_entries.isEmpty()) {
                    emit dataChanged(index(0, 0), index(m_entries.size() - 1, 0), {ThumbnailPathRole});
                }
            });
    watcher->setFuture(future);
}

void FolderCompareSideModel::applyFilterAndSort(QVector<CompareEntry> &entries) const
{
    if (m_hideIdentical) {
        entries.erase(std::remove_if(entries.begin(), entries.end(),
                                     [](const CompareEntry &entry) {
                                         return entry.status == StatusIdentical;
                                     }),
                      entries.end());
    }
    FolderFilterSortUtils::applyNameJunkSizeFilter<CompareEntry>(
        entries, m_filterSettings, m_attributeCache.imageSizes(),
        [](const CompareEntry &entry) { return viewForCompareEntry(entry); });
    FolderFilterSortUtils::sortEntries<CompareEntry>(
        entries, m_filterSettings, [](const CompareEntry &entry) { return viewForCompareEntry(entry); },
        [this](const QString &path, quint64 *value) {
            return m_attributeCache.signatureHashForPath(path, value);
        });
}

void FolderCompareSideModel::notifySelectionChanged()
{
    rebuildSelectedPaths();
    emit selectedPathsChanged();
    const CompareEntry *entry = m_selectionManager.selectedKeys().size() == 1
        ? findEntryById(m_entries, m_selectionManager.selectedKeys().first())
        : nullptr;
    const bool nextIsImage = entry && entry->isImage && !entry->isGhost;
    const bool nextIsVideo = entry && entry->isVideo && !entry->isGhost;
    if (nextIsImage != m_selectedIsImage) {
        m_selectedIsImage = nextIsImage;
        emit selectedIsImageChanged();
    }
    if (nextIsVideo != m_selectedIsVideo) {
        m_selectedIsVideo = nextIsVideo;
        emit selectedIsVideoChanged();
    }
    updateSelectionTotalsAsync();
    if (!m_entries.isEmpty()) {
        emit dataChanged(index(0, 0), index(m_entries.size() - 1, 0), {SelectedRole});
    }
}

void FolderCompareSideModel::updateSelectionTotalsAsync()
{
    SelectionStatisticsUtils::updateSelectionTotalsAsync(
        this, m_selectedPaths, &m_selectionTotalsGeneration,
        [this](int fileCount, qint64 totalBytes) { setSelectionTotals(fileCount, totalBytes); });
}

void FolderCompareSideModel::setSelectionTotals(int fileCount, qint64 totalBytes)
{
    if (m_selectedFileCount != fileCount) {
        m_selectedFileCount = fileCount;
        emit selectedFileCountChanged();
    }
    if (m_selectedTotalBytes != totalBytes) {
        m_selectedTotalBytes = totalBytes;
        emit selectedTotalBytesChanged();
    }
}

void FolderCompareSideModel::rebuildSelectedPaths()
{
    QStringList nextPaths;
    nextPaths.reserve(m_selectionManager.selectedKeys().size());
    for (const QString &id : m_selectionManager.selectedKeys()) {
        const CompareEntry *entry = findEntryById(m_entries, id);
        if (entry && !entry->filePath.isEmpty() && !entry->isGhost) {
            nextPaths.append(entry->filePath);
        }
    }
    m_selectedPaths = nextPaths;
}
