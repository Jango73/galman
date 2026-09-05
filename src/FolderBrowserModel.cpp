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

#include "FolderBrowserModel.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QImageReader>
#include <QLocale>
#include <QtConcurrent>
#include <QDebug>
#include <QThread>

#include "ApplicationSettings.h"
#include "FolderEntryView.h"
#include "FolderFilterSortUtils.h"
#include "ImageMetadataUtils.h"
#include "PlatformUtils.h"
#include "RowMatchUtils.h"
#include "SelectionStatisticsUtils.h"
#include "VideoThumbnailUtils.h"

namespace {

const QSet<QString> &supportedImageFormats()
{
    static const QSet<QString> formats = []() {
        QSet<QString> set;
        for (const QByteArray &format : QImageReader::supportedImageFormats()) {
            set.insert(QString::fromLatin1(format).toLower());
        }
        return set;
    }();
    return formats;
}

bool isImagePath(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    const QFileInfo info(path);
    if (!info.exists() || info.isDir()) {
        return false;
    }
    return supportedImageFormats().contains(info.suffix().toLower());
}

bool isImageInfo(const QFileInfo &info)
{
    if (!info.exists() || info.isDir()) {
        return false;
    }
    return supportedImageFormats().contains(info.suffix().toLower());
}

QString previewPathForInfo(const QFileInfo &info)
{
    return info.absoluteFilePath();
}

QString thumbnailRevisionForFileInfo(const QFileInfo &info)
{
    if (!info.exists() || info.isDir()) {
        return QString();
    }
    return QStringLiteral("%1:%2")
        .arg(QString::number(info.lastModified().toMSecsSinceEpoch()))
        .arg(QString::number(info.size()));
}

QString findReplacementPathForSelection(const QString &originalPath, const QVector<QFileInfo> &entries)
{
    const QFileInfo originalInfo(originalPath);
    if (originalPath.isEmpty() || originalInfo.isDir()) {
        return QString();
    }
    const QString originalFolder = originalInfo.absolutePath();
    const QString originalBaseName = originalInfo.completeBaseName();
    if (originalFolder.isEmpty() || originalBaseName.isEmpty()) {
        return QString();
    }
    for (const QFileInfo &entry : entries) {
        if (entry.isDir() || entry.absolutePath() != originalFolder) {
            continue;
        }
        if (entry.completeBaseName() == originalBaseName) {
            return entry.absoluteFilePath();
        }
    }
    return QString();
}

bool fileInfoEquivalent(const QFileInfo &left, const QFileInfo &right)
{
    return left.absoluteFilePath() == right.absoluteFilePath() && left.fileName() == right.fileName()
        && left.isDir() == right.isDir() && left.suffix() == right.suffix()
        && left.birthTime() == right.birthTime() && left.lastModified() == right.lastModified();
}

FolderEntryView::EntryView viewForFileInfo(const QFileInfo &info)
{
    FolderEntryView::EntryView view;
    view.fileName = info.fileName();
    view.filePath = info.absoluteFilePath();
    view.isFolder = info.isDir();
    view.isGhost = false;
    view.createdTime = info.birthTime().isValid() ? info.birthTime() : info.lastModified();
    view.modifiedTime = info.lastModified();
    view.isImageFlag = isImageInfo(info);
    view.isVideoFlag = VideoThumbnailUtils::isVideoFile(info);
    view.byteSize = info.isDir() ? -1 : info.size();
    return view;
}

} // namespace

FolderBrowserModel::FolderBrowserModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_transferController(this)
    , m_attributeCache(this)
{
    qInfo() << "FolderBrowserModel::FolderBrowserModel";
    m_refreshTimer.setInterval(150);
    m_refreshTimer.setSingleShot(true);
    connect(&m_refreshTimer, &QTimer::timeout, this, &FolderBrowserModel::refresh);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        if (m_transferController.copyInProgress() || m_transferController.trashInProgress()) {
            m_pendingWatcherRefresh = true;
            return;
        }
        scheduleRefresh();
    });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        if (m_transferController.copyInProgress() || m_transferController.trashInProgress()) {
            m_pendingWatcherRefresh = true;
            return;
        }
        scheduleRefresh();
    });
    connect(&m_transferController, &FolderTransferController::copyInProgressChanged, this, [this]() {
        emit copyInProgressChanged();
        if (!m_transferController.copyInProgress() && !m_transferController.trashInProgress()
            && m_pendingWatcherRefresh) {
            m_pendingWatcherRefresh = false;
            scheduleRefresh();
        }
    });
    connect(&m_transferController, &FolderTransferController::copyProgressChanged, this, [this]() {
        emit copyProgressChanged();
    });
    connect(&m_transferController, &FolderTransferController::copyFinished, this,
            [this](const QVariantMap &result) {
                emit copyFinished(result);
            });
    connect(&m_transferController, &FolderTransferController::trashInProgressChanged, this, [this]() {
        emit trashInProgressChanged();
        if (!m_transferController.trashInProgress() && !m_transferController.copyInProgress()
            && m_pendingWatcherRefresh) {
            m_pendingWatcherRefresh = false;
            scheduleRefresh();
        }
    });
    connect(&m_transferController, &FolderTransferController::trashProgressChanged, this, [this]() {
        emit trashProgressChanged();
    });
    connect(&m_transferController, &FolderTransferController::trashFinished, this,
            [this](const QVariantMap &result) {
                emit trashFinished(result);
                if (result.value("moved").toInt() > 0) {
                    refresh();
                }
                clearSelection();
            });
    setRootPath(QDir::homePath());
}

QString FolderBrowserModel::rootPath() const
{
    return m_rootPath;
}

void FolderBrowserModel::setRootPath(const QString &path)
{
    qInfo() << "FolderBrowserModel::setRootPath" << path;
    if (path.isEmpty() || path == m_rootPath) {
        return;
    }
    m_rootPath = path;
    emit rootPathChanged();
    if (!m_settingsKey.isEmpty()) {
        ApplicationSettings settings;
        settings.setValue(m_settingsKey, m_rootPath);
    }
    const QStringList watched = m_watcher.directories();
    if (!watched.isEmpty()) {
        m_watcher.removePaths(watched);
    }
    if (!m_watcher.files().isEmpty()) {
        m_watcher.removePaths(m_watcher.files());
    }
    if (QDir(m_rootPath).exists()) {
        m_watcher.addPath(m_rootPath);
    }
    refresh();
    clearSelection();
}

QString FolderBrowserModel::settingsKey() const
{
    return m_settingsKey;
}

void FolderBrowserModel::setSettingsKey(const QString &key)
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
    restoreViewSettings();
}

QString FolderBrowserModel::nameFilter() const
{
    return m_filterSettings.nameFilter();
}

void FolderBrowserModel::setNameFilter(const QString &filter)
{
    qInfo() << "FolderBrowserModel::setNameFilter" << filter;
    if (filter == m_filterSettings.nameFilter()) {
        return;
    }
    m_filterSettings.setNameFilter(filter);
    emit nameFilterChanged();
    rebuildEntries();
}

FolderBrowserModel::SortKey FolderBrowserModel::sortKey() const
{
    return static_cast<SortKey>(m_filterSettings.sortKeyValue());
}

void FolderBrowserModel::setSortKey(FolderBrowserModel::SortKey key)
{
    if (static_cast<int>(key) == m_filterSettings.sortKeyValue()) {
        return;
    }
    m_filterSettings.setSortKeyValue(static_cast<int>(key));
    emit sortKeyChanged();
    rebuildEntries();
}

Qt::SortOrder FolderBrowserModel::sortOrder() const
{
    return m_filterSettings.sortOrder();
}

void FolderBrowserModel::setSortOrder(Qt::SortOrder order)
{
    if (order == m_filterSettings.sortOrder()) {
        return;
    }
    m_filterSettings.setSortOrder(order);
    emit sortOrderChanged();
    rebuildEntries();
}

bool FolderBrowserModel::showDirsFirst() const
{
    return m_filterSettings.showFoldersFirst();
}

QStringList FolderBrowserModel::junkExtensions()
{
    return FolderFilterSettings::junkExtensions();
}

QString FolderBrowserModel::junkExtensionsString() const
{
    return FolderFilterSettings::junkExtensionsString();
}

void FolderBrowserModel::setJunkExtensionsList(const QString &extensions)
{
    FolderFilterSettings::setJunkExtensionsList(extensions);
}

bool FolderBrowserModel::hideJunkFiles() const
{
    return m_filterSettings.hideJunkFiles();
}

void FolderBrowserModel::setHideJunkFiles(bool enabled)
{
    if (enabled == m_filterSettings.hideJunkFiles()) {
        return;
    }
    m_filterSettings.setHideJunkFiles(enabled);
    emit hideJunkFilesChanged();
    rebuildEntries();
}

void FolderBrowserModel::setShowDirsFirst(bool enabled)
{
    qInfo() << "FolderBrowserModel::setShowDirsFirst" << enabled;
    if (enabled == m_filterSettings.showFoldersFirst()) {
        return;
    }
    m_filterSettings.setShowFoldersFirst(enabled);
    emit showDirsFirstChanged();
    rebuildEntries();
}

bool FolderBrowserModel::loading() const
{
    return m_loading;
}

qint64 FolderBrowserModel::minimumByteSize() const
{
    return m_filterSettings.minimumByteSize();
}

void FolderBrowserModel::setMinimumByteSize(qint64 value)
{
    const qint64 normalized = FolderFilterSettings::normalizedByteSize(value);
    if (normalized == m_filterSettings.minimumByteSize()) {
        return;
    }
    m_filterSettings.setMinimumByteSize(value);
    emit minimumByteSizeChanged();
    rebuildEntries();
}

qint64 FolderBrowserModel::maximumByteSize() const
{
    return m_filterSettings.maximumByteSize();
}

void FolderBrowserModel::setMaximumByteSize(qint64 value)
{
    const qint64 normalized = FolderFilterSettings::normalizedByteSize(value);
    if (normalized == m_filterSettings.maximumByteSize()) {
        return;
    }
    m_filterSettings.setMaximumByteSize(value);
    emit maximumByteSizeChanged();
    rebuildEntries();
}

int FolderBrowserModel::minimumImageWidth() const
{
    return m_filterSettings.minimumImageWidth();
}

void FolderBrowserModel::setMinimumImageWidth(int value)
{
    if (FolderFilterSettings::normalizedDimension(value) == m_filterSettings.minimumImageWidth()) {
        return;
    }
    m_filterSettings.setMinimumImageWidth(value);
    emit minimumImageWidthChanged();
    rebuildEntries();
}

int FolderBrowserModel::maximumImageWidth() const
{
    return m_filterSettings.maximumImageWidth();
}

void FolderBrowserModel::setMaximumImageWidth(int value)
{
    if (FolderFilterSettings::normalizedDimension(value) == m_filterSettings.maximumImageWidth()) {
        return;
    }
    m_filterSettings.setMaximumImageWidth(value);
    emit maximumImageWidthChanged();
    rebuildEntries();
}

int FolderBrowserModel::minimumImageHeight() const
{
    return m_filterSettings.minimumImageHeight();
}

void FolderBrowserModel::setMinimumImageHeight(int value)
{
    if (FolderFilterSettings::normalizedDimension(value) == m_filterSettings.minimumImageHeight()) {
        return;
    }
    m_filterSettings.setMinimumImageHeight(value);
    emit minimumImageHeightChanged();
    rebuildEntries();
}

int FolderBrowserModel::maximumImageHeight() const
{
    return m_filterSettings.maximumImageHeight();
}

void FolderBrowserModel::setMaximumImageHeight(int value)
{
    if (FolderFilterSettings::normalizedDimension(value) == m_filterSettings.maximumImageHeight()) {
        return;
    }
    m_filterSettings.setMaximumImageHeight(value);
    emit maximumImageHeightChanged();
    rebuildEntries();
}

QStringList FolderBrowserModel::selectedPaths() const
{
    return m_selectionManager.selectedKeys();
}

bool FolderBrowserModel::selectedIsImage() const
{
    return m_selectedIsImage;
}

bool FolderBrowserModel::selectedIsVideo() const
{
    return m_selectedIsVideo;
}

int FolderBrowserModel::selectedFileCount() const
{
    return m_selectedFileCount;
}

qint64 FolderBrowserModel::selectedTotalBytes() const
{
    return m_selectedTotalBytes;
}

bool FolderBrowserModel::copyInProgress() const
{
    return m_transferController.copyInProgress();
}

qreal FolderBrowserModel::copyProgress() const
{
    return m_transferController.copyProgress();
}

bool FolderBrowserModel::trashInProgress() const
{
    return m_transferController.trashInProgress();
}

qreal FolderBrowserModel::trashProgress() const
{
    return m_transferController.trashProgress();
}

int FolderBrowserModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant FolderBrowserModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const QFileInfo &info = m_entries.at(index.row());
    switch (role) {
    case FileNameRole:
        return info.fileName();
    case FilePathRole:
        return info.absoluteFilePath();
    case IsDirRole:
        return info.isDir();
    case IsImageRole:
        return isImageInfo(info);
    case IsVideoRole:
        return VideoThumbnailUtils::isVideoFile(info);
    case ThumbnailPathRole: {
        const QString path = previewPathForInfo(info);
        if (path.isEmpty()) {
            return QString();
        }
        if (isImageInfo(info)) {
            return path;
        }
        if (VideoThumbnailUtils::isVideoFile(info)) {
            return m_attributeCache.videoThumbnails().value(path);
        }
        return QString();
    }
    case ThumbnailRevisionRole:
        return thumbnailRevisionForFileInfo(info);
    case SuffixRole:
        return info.suffix();
    case CreatedRole:
        return info.birthTime().isValid() ? info.birthTime() : info.lastModified();
    case ModifiedRole:
        return info.lastModified();
    case SelectedRole:
        return m_selectionManager.isSelected(info.absoluteFilePath());
    case CompareStatusRole:
        return 0;
    case GhostRole:
        return false;
    default:
        return {};
    }
}

QHash<int, QByteArray> FolderBrowserModel::roleNames() const
{
    return {{FileNameRole, "fileName"},
            {FilePathRole, "filePath"},
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
            {GhostRole, "isGhost"}};
}

void FolderBrowserModel::refresh()
{
    qInfo() << "FolderBrowserModel::refresh" << m_rootPath;
    if (m_rootPath.isEmpty()) {
        return;
    }
    setLoading(true);
    const int token = ++m_generation;
    const QString path = m_rootPath;
    auto future = QtConcurrent::run([path]() {
        return QDir(path).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::NoSort).toVector();
    });
    auto *watcher = new QFutureWatcher<QVector<QFileInfo>>(this);
    connect(watcher, &QFutureWatcher<QVector<QFileInfo>>::finished, this, [this, watcher, token]() {
        const QVector<QFileInfo> entries = watcher->result();
        watcher->deleteLater();
        if (token != m_generation) {
            return;
        }
        m_baseEntries = entries;
        pruneCaches(m_baseEntries);
        updateFileWatchers(m_baseEntries);
        rebuildEntries();
        setLoading(false);
    });
    watcher->setFuture(future);
}

void FolderBrowserModel::activate(int row)
{
    qInfo() << "FolderBrowserModel::activate" << row;
    if (row < 0 || row >= m_entries.size()) {
        return;
    }
    const QFileInfo &info = m_entries.at(row);
    if (info.isDir()) {
        emit folderActivated(info.absoluteFilePath());
    } else {
        emit fileActivated(info.absoluteFilePath());
    }
}

QString FolderBrowserModel::keyForRow(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return QString();
    }
    return m_entries.at(row).absoluteFilePath();
}

void FolderBrowserModel::select(int row, bool multi)
{
    const QString key = keyForRow(row);
    if (key.isEmpty()) {
        return;
    }
    bool changed = false;
    if (!multi) {
        changed = m_selectionManager.selectSingle(key);
    } else {
        changed = m_selectionManager.toggleKey(key);
    }
    if (changed) {
        notifySelectionChanged();
    }
}

bool FolderBrowserModel::isSelected(int row) const
{
    return m_selectionManager.isSelected(keyForRow(row));
}

bool FolderBrowserModel::isDir(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return false;
    }
    return m_entries.at(row).isDir();
}

QString FolderBrowserModel::pathForRow(int row) const
{
    return keyForRow(row);
}

int FolderBrowserModel::rowForPrefix(const QString &prefix, int startRow) const
{
    return RowMatchUtils::rowForPrefix(
        prefix, startRow, m_entries.size(), [this](int row) { return m_entries.at(row).fileName(); });
}

QString FolderBrowserModel::modifiedForRow(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return {};
    }
    return QLocale().toString(m_entries.at(row).lastModified(), QLocale::ShortFormat);
}

int FolderBrowserModel::copyNameConflictCount(const QString &targetFolder) const
{
    return FolderTransferController::countNameConflicts(m_selectionManager.selectedKeys(), targetFolder);
}

void FolderBrowserModel::clearSelection()
{
    if (m_selectionManager.clearSelection()) {
        notifySelectionChanged();
    }
}

void FolderBrowserModel::goUp()
{
    qInfo() << "FolderBrowserModel::goUp" << m_rootPath;
    if (m_rootPath.isEmpty()) {
        return;
    }
    QDir folder(m_rootPath);
    if (folder.isRoot()) {
        return;
    }
    m_pendingSelectionPath = m_rootPath;
    folder.cdUp();
    setRootPath(folder.absolutePath());
}

void FolderBrowserModel::refreshFiles(const QStringList &paths)
{
    qInfo() << "FolderBrowserModel::refreshFiles" << paths.size();
    if (m_rootPath.isEmpty() || paths.isEmpty()) {
        return;
    }
    const QString rootPath = QDir::cleanPath(m_rootPath);
    QVector<QFileInfo> nextEntries = m_baseEntries;
    bool changed = false;
    for (const QString &path : paths) {
        const QString cleanPath = QDir::cleanPath(path);
        if (cleanPath.isEmpty() || !cleanPath.startsWith(rootPath)) {
            continue;
        }
        if (cleanPath.size() > rootPath.size() && cleanPath.at(rootPath.size()) != QLatin1Char('/')) {
            continue;
        }
        const QFileInfo info(cleanPath);
        int existingIndex = -1;
        for (int index = 0; index < nextEntries.size(); ++index) {
            if (nextEntries.at(index).absoluteFilePath() == cleanPath) {
                existingIndex = index;
                break;
            }
        }
        if (info.exists()) {
            if (existingIndex < 0) {
                nextEntries.append(info);
            } else {
                nextEntries[existingIndex] = info;
            }
            changed = true;
        } else if (existingIndex >= 0) {
            nextEntries.remove(existingIndex);
            changed = true;
        }
    }
    if (!changed) {
        return;
    }
    m_baseEntries = nextEntries;
    pruneCaches(m_baseEntries);
    updateFileWatchers(m_baseEntries);
    rebuildEntries();
}

void FolderBrowserModel::setSelection(const QVariantList &rows, bool additive)
{
    if (m_selectionManager.setFromRowsGeneric(
            rows, additive, m_entries.size(), [this](int row) { return keyForRow(row); })) {
        notifySelectionChanged();
    }
}

void FolderBrowserModel::setSelectionRange(int start, int end, bool additive)
{
    if (m_entries.isEmpty()) {
        return;
    }
    const int from = std::max(0, std::min(start, end));
    const int to = std::min(end > start ? end : start, static_cast<int>(m_entries.size()) - 1);
    const int lower = std::min(from, to);
    const int upper = std::max(from, to);
    QVariantList rows;
    rows.reserve(upper - lower + 1);
    for (int row = lower; row <= upper; ++row) {
        rows.append(row);
    }
    setSelection(rows, additive);
}

bool FolderBrowserModel::allSelected() const
{
    return m_selectionManager.allSelected(m_entries.size());
}

QVariantList FolderBrowserModel::selectedRows() const
{
    return m_selectionManager.selectedRows(m_entries.size(),
                                           [this](int row) { return keyForRow(row); });
}

bool FolderBrowserModel::isImage(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return false;
    }
    return isImagePath(m_entries.at(row).absoluteFilePath());
}

bool FolderBrowserModel::isVideo(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return false;
    }
    return VideoThumbnailUtils::isVideoFile(m_entries.at(row));
}

int FolderBrowserModel::selectedCompareStatus() const
{
    return 0;
}

bool FolderBrowserModel::selectedIsGhost() const
{
    return false;
}

bool FolderBrowserModel::selectedIsNewer() const
{
    return false;
}

QVariantMap FolderBrowserModel::selectionStats() const
{
    QVariantMap result;
    const auto stats = SelectionStatisticsUtils::computeStatistics(m_selectionManager.selectedKeys());
    result.insert("dirs", stats.folderCount);
    result.insert("files", stats.fileCount);
    return result;
}

QVariantMap FolderBrowserModel::copySelectedTo(const QString &targetFolder)
{
    qInfo() << "FolderBrowserModel::copySelectedTo" << targetFolder;
    return m_transferController.copySelectedPaths(m_selectionManager.selectedKeys(), targetFolder,
                                                  "FolderBrowserModel");
}

void FolderBrowserModel::startTransferSelectedTo(const QString &targetFolder, bool moveItems)
{
    qInfo() << "FolderBrowserModel::startTransferSelectedTo" << targetFolder << moveItems;
    m_transferController.startTransferPaths(m_selectionManager.selectedKeys(), targetFolder, moveItems, 0, {});
}

void FolderBrowserModel::startCopySelectedTo(const QString &targetFolder)
{
    startTransferSelectedTo(targetFolder, false);
}

void FolderBrowserModel::startMoveSelectedTo(const QString &targetFolder)
{
    startTransferSelectedTo(targetFolder, true);
}

void FolderBrowserModel::cancelCopy()
{
    m_transferController.cancelCopy();
}

QVariantMap FolderBrowserModel::moveSelectedToTrash()
{
    qInfo() << "FolderBrowserModel::moveSelectedToTrash";
    return requestRemoval(true);
}

void FolderBrowserModel::startMoveSelectedToTrash()
{
    startRemoval(true);
}

QVariantMap FolderBrowserModel::deleteSelectedPermanently()
{
    return requestRemoval(false);
}

void FolderBrowserModel::startDeleteSelectedPermanently()
{
    startRemoval(false);
}

QVariantMap FolderBrowserModel::requestRemoval(bool moveToTrash)
{
    return m_transferController.requestRemovalPaths(m_selectionManager.selectedKeys(), moveToTrash);
}

void FolderBrowserModel::startRemoval(bool moveToTrash)
{
    m_transferController.startRemovalPaths(m_selectionManager.selectedKeys(), moveToTrash);
}

void FolderBrowserModel::cancelTrash()
{
    m_transferController.cancelTrash();
}

QVariantMap FolderBrowserModel::renamePath(const QString &path, const QString &newName)
{
    qInfo() << "FolderBrowserModel::renamePath" << path << newName;
    QVariantMap result;
    result.insert("ok", false);
    QString targetPath;
    QString error;
    if (!PlatformUtils::renamePath(path, newName, &targetPath, &error)) {
        result.insert("error", error.isEmpty() ? tr("Rename failed") : error);
        return result;
    }
    QStringList nextSelection = m_selectionManager.selectedKeys();
    const int selectedIndex = nextSelection.indexOf(path);
    if (selectedIndex >= 0) {
        nextSelection[selectedIndex] = targetPath;
        m_selectionManager.setSelectedKeys(nextSelection);
        notifySelectionChanged();
    }
    refreshFiles({path, targetPath});
    result.insert("ok", true);
    result.insert("newPath", targetPath);
    return result;
}

bool FolderBrowserModel::createFolder(const QString &parentPath, const QString &folderName)
{
    qInfo() << "FolderBrowserModel::createFolder" << parentPath << folderName;
    if (parentPath.isEmpty() || folderName.isEmpty()) {
        return false;
    }
    QDir folder(parentPath);
    if (!folder.exists()) {
        return false;
    }
    return folder.mkdir(folderName);
}

bool FolderBrowserModel::hasGhostOnOtherSide(int row) const
{
    Q_UNUSED(row)
    return false;
}

void FolderBrowserModel::setLoading(bool loading)
{
    if (loading == m_loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

void FolderBrowserModel::scheduleRefresh()
{
    if (!m_refreshTimer.isActive()) {
        m_refreshTimer.start();
    }
}

void FolderBrowserModel::updateFileWatchers(const QVector<QFileInfo> &entries)
{
    QStringList nextFiles;
    nextFiles.reserve(entries.size());
    for (const QFileInfo &info : entries) {
        if (info.isFile()) {
            nextFiles.append(info.absoluteFilePath());
        }
    }
    if (!m_watcher.files().isEmpty()) {
        m_watcher.removePaths(m_watcher.files());
    }
    if (!nextFiles.isEmpty()) {
        Q_UNUSED(m_watcher.addPaths(nextFiles));
    }
}

void FolderBrowserModel::applyEntriesIncremental(const QVector<QFileInfo> &entries)
{
    const QVector<int> dataRoles = {FileNameRole, FilePathRole,      IsDirRole,   IsImageRole,
                                    IsVideoRole,  ThumbnailPathRole, SuffixRole,  CreatedRole,
                                    ModifiedRole, CompareStatusRole, GhostRole};
    int position = 0;
    while (position < entries.size()) {
        const QString nextPath = entries.at(position).absoluteFilePath();
        if (position < m_entries.size() && m_entries.at(position).absoluteFilePath() == nextPath) {
            m_entries[position] = entries.at(position);
            emit dataChanged(index(position, 0), index(position, 0), dataRoles);
            position += 1;
            continue;
        }
        int existing = -1;
        for (int search = position + 1; search < m_entries.size(); ++search) {
            if (m_entries.at(search).absoluteFilePath() == nextPath) {
                existing = search;
                break;
            }
        }
        if (existing >= 0) {
            beginMoveRows(QModelIndex(), existing, existing, QModelIndex(), position);
            const QFileInfo moved = m_entries.takeAt(existing);
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
}

bool FolderBrowserModel::byteSizeFiltersActive() const
{
    return m_filterSettings.byteSizeFiltersActive();
}

bool FolderBrowserModel::imageSizeFiltersActive() const
{
    return m_filterSettings.imageSizeFiltersActive();
}

bool FolderBrowserModel::signatureSortActive() const
{
    return m_filterSettings.sortKeyValue() == FolderFilterSettings::SortBySignature;
}

void FolderBrowserModel::pruneCaches(const QVector<QFileInfo> &entries)
{
    QSet<QString> currentPaths;
    currentPaths.reserve(entries.size());
    for (const QFileInfo &info : entries) {
        currentPaths.insert(info.absoluteFilePath());
    }
    m_attributeCache.pruneCaches(currentPaths);
}

void FolderBrowserModel::rebuildEntries()
{
    if (imageSizeFiltersActive()) {
        requestImageSizeRefresh();
    }
    if (signatureSortActive()) {
        requestSignatureHashRefresh();
    }
    requestVideoThumbnailRefresh();
    QVector<QFileInfo> filtered = m_baseEntries;
    applyFilterAndSort(filtered);
    applyEntriesIncremental(filtered);
    QStringList nextSelection;
    if (!m_pendingSelectionPath.isEmpty()) {
        for (const QFileInfo &entry : m_entries) {
            if (entry.absoluteFilePath() == m_pendingSelectionPath) {
                nextSelection.append(m_pendingSelectionPath);
                break;
            }
        }
        m_pendingSelectionPath.clear();
    } else {
        for (const QString &path : m_selectionManager.selectedKeys()) {
            if (QFileInfo::exists(path)) {
                nextSelection.append(path);
                continue;
            }
            const QString replacement = findReplacementPathForSelection(path, m_entries);
            if (!replacement.isEmpty() && !nextSelection.contains(replacement)) {
                nextSelection.append(replacement);
            }
        }
    }
    if (nextSelection != m_selectionManager.selectedKeys()) {
        m_selectionManager.setSelectedKeys(nextSelection);
        notifySelectionChanged();
    }
    saveViewSettings();
}

void FolderBrowserModel::saveViewSettings() const
{
    if (m_settingsKey.isEmpty()) {
        return;
    }
    m_filterSettings.saveViewSettings(m_settingsKey + QStringLiteral("/view"));
}

void FolderBrowserModel::restoreViewSettings()
{
    qInfo() << "FolderBrowserModel::restoreViewSettings" << m_settingsKey;
    if (m_settingsKey.isEmpty()) {
        return;
    }
    FolderFilterSettings stored;
    stored.loadViewSettings(m_settingsKey + QStringLiteral("/view"));
    setNameFilter(stored.nameFilter());
    const int sortKey = stored.sortKeyValue();
    if (sortKey >= Name && sortKey <= Signature) {
        setSortKey(static_cast<SortKey>(sortKey));
    }
    const int sortOrder = static_cast<int>(stored.sortOrder());
    if (sortOrder == Qt::AscendingOrder || sortOrder == Qt::DescendingOrder) {
        setSortOrder(static_cast<Qt::SortOrder>(sortOrder));
    }
    setShowDirsFirst(stored.showFoldersFirst());
    setHideJunkFiles(stored.hideJunkFiles());
    setMinimumByteSize(stored.minimumByteSize());
    setMaximumByteSize(stored.maximumByteSize());
    setMinimumImageWidth(stored.minimumImageWidth());
    setMaximumImageWidth(stored.maximumImageWidth());
    setMinimumImageHeight(stored.minimumImageHeight());
    setMaximumImageHeight(stored.maximumImageHeight());
}

void FolderBrowserModel::requestImageSizeRefresh()
{
    if (m_attributeCache.imageSizeLoading() || !imageSizeFiltersActive()) {
        return;
    }
    QStringList pending;
    for (const QFileInfo &info : m_baseEntries) {
        if (info.isDir() || !isImageInfo(info)) {
            continue;
        }
        const QString path = info.absoluteFilePath();
        if (m_attributeCache.imageSizeAttempted(path)) {
            continue;
        }
        pending.append(path);
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

void FolderBrowserModel::requestVideoThumbnailRefresh()
{
    if (m_attributeCache.videoThumbnailLoading()) {
        return;
    }
    QStringList pending;
    for (const QFileInfo &info : m_baseEntries) {
        if (info.isDir() || !VideoThumbnailUtils::isVideoFile(info)) {
            continue;
        }
        const QString path = info.absoluteFilePath();
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
        if (m_attributeCache.videoThumbnailAttempted(path)) {
            continue;
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

void FolderBrowserModel::requestSignatureHashRefresh()
{
    if (m_attributeCache.signatureLoading() || !signatureSortActive()) {
        return;
    }
    QStringList pending;
    for (const QFileInfo &info : m_baseEntries) {
        if (info.isDir() || !isImageInfo(info)) {
            continue;
        }
        const QString path = info.absoluteFilePath();
        if (m_attributeCache.signatureAttempted(path)) {
            continue;
        }
        pending.append(path);
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

void FolderBrowserModel::applyFilterAndSort(QVector<QFileInfo> &entries) const
{
    FolderFilterSortUtils::applyNameJunkSizeFilter<QFileInfo>(
        entries, m_filterSettings, m_attributeCache.imageSizes(),
        [](const QFileInfo &info) { return viewForFileInfo(info); });
    FolderFilterSortUtils::sortEntries<QFileInfo>(
        entries, m_filterSettings, [](const QFileInfo &info) { return viewForFileInfo(info); },
        [this](const QString &path, quint64 *value) {
            return m_attributeCache.signatureHashForPath(path, value);
        });
}

void FolderBrowserModel::notifySelectionChanged()
{
    qInfo() << "FolderBrowserModel::notifySelectionChanged" << m_selectionManager.selectedKeys().size();
    emit selectedPathsChanged();
    const QStringList keys = m_selectionManager.selectedKeys();
    const bool nextIsImage = keys.size() == 1 && isImagePath(keys.first());
    const bool nextIsVideo = keys.size() == 1 && VideoThumbnailUtils::isVideoFile(QFileInfo(keys.first()));
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

void FolderBrowserModel::updateSelectionTotalsAsync()
{
    SelectionStatisticsUtils::updateSelectionTotalsAsync(
        this, m_selectionManager.selectedKeys(), &m_selectionTotalsGeneration,
        [this](int fileCount, qint64 totalBytes) { setSelectionTotals(fileCount, totalBytes); });
}

void FolderBrowserModel::setSelectionTotals(int fileCount, qint64 totalBytes)
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
