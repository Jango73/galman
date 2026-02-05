
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

#include <QCollator>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QImageReader>
#include <QFile>
#include <QDebug>
#include <QLocale>
#include <QtConcurrent>
#include <algorithm>
#include <QSet>
#include <QThread>

#include "CopyWorker.h"
#include "FileOperationUtils.h"
#include "ImageMetadataUtils.h"
#include "PlatformUtils.h"
#include "RowMatchUtils.h"
#include "SelectionStatisticsUtils.h"
#include "TrashWorker.h"

namespace {

/**
 * @brief Returns the set of supported image file extensions.
 * @return Set of lowercase image file extensions.
 */
const QSet<QString> &supportedImageFormats()
{
    static const QSet<QString> formats = []() {
        QSet<QString> set;
        const auto supported = QImageReader::supportedImageFormats();
        for (const QByteArray &fmt : supported) {
            set.insert(QString::fromLatin1(fmt).toLower());
        }
        return set;
    }();
    return formats;
}

/**
 * @brief Checks whether a path points to a supported image file.
 * @param path File path to inspect.
 * @return True if the path is an existing image file, false otherwise.
 */
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

/**
 * @brief Checks whether a file info entry is a supported image file.
 * @param info File info entry to inspect.
 * @return True if the entry is a supported image file, false otherwise.
 */
bool isImageInfo(const QFileInfo &info)
{
    if (!info.exists() || info.isDir()) {
        return false;
    }
    return supportedImageFormats().contains(info.suffix().toLower());
}

} // namespace

namespace {

/**
 * @brief Compares two QFileInfo entries for equality in key attributes.
 * @param left First file info to compare.
 * @param right Second file info to compare.
 * @return True when the entries match on path, name, type, and timestamps.
 */
bool fileInfoEquivalent(const QFileInfo &left, const QFileInfo &right)
{
    return left.absoluteFilePath() == right.absoluteFilePath()
        && left.fileName() == right.fileName()
        && left.isDir() == right.isDir()
        && left.suffix() == right.suffix()
        && left.birthTime() == right.birthTime()
        && left.lastModified() == right.lastModified();
}

struct FolderBrowserTrashConstants {
    static constexpr int emptyCount = 0;
    static constexpr qreal zeroProgress = 0.0;
    static constexpr bool pendingTrue = true;
};

struct FolderBrowserFilterConstants {
    static constexpr qint64 unsetByteSize = -1;
    static constexpr int unsetDimension = -1;
    static constexpr int signatureDimension = 32;
};

} // namespace

/**
 * @brief Constructs the folder browser model and wires file watchers.
 * @param parent Parent QObject for ownership.
 */
FolderBrowserModel::FolderBrowserModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_refreshTimer.setInterval(150);
    m_refreshTimer.setSingleShot(true);
    connect(&m_refreshTimer, &QTimer::timeout, this, &FolderBrowserModel::refresh);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        if (m_copyInProgress || m_trashInProgress) {
            m_pendingWatcherRefresh = true;
            return;
        }
        scheduleRefresh();
    });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        if (m_copyInProgress || m_trashInProgress) {
            m_pendingWatcherRefresh = true;
            return;
        }
        scheduleRefresh();
    });

    setRootPath(QDir::homePath());
}

/**
 * @brief Returns the current root path.
 * @return Root folder path.
 */
QString FolderBrowserModel::rootPath() const
{
    return m_rootPath;
}

/**
 * @brief Sets the root path and refreshes the model.
 * @param path Root folder path to set.
 */
void FolderBrowserModel::setRootPath(const QString &path)
{
    if (path.isEmpty() || path == m_rootPath) {
        return;
    }

    m_rootPath = path;
    emit rootPathChanged();
    if (!m_settingsKey.isEmpty()) {
        QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Galman", "Galman");
        settings.setValue(m_settingsKey, m_rootPath);
    }

    const QStringList watched = m_watcher.directories();
    if (!watched.isEmpty()) {
        m_watcher.removePaths(watched);
    }
    const QStringList watchedFiles = m_watcher.files();
    if (!watchedFiles.isEmpty()) {
        m_watcher.removePaths(watchedFiles);
    }
    if (QDir(m_rootPath).exists()) {
        m_watcher.addPath(m_rootPath);
    }

    refresh();
    clearSelection();
}

/**
 * @brief Returns the settings key used to persist the root path.
 * @return Settings key string.
 */
QString FolderBrowserModel::settingsKey() const
{
    return m_settingsKey;
}

/**
 * @brief Sets the settings key and loads a stored root path if available.
 * @param key Settings key string to use for persistence.
 */
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
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Galman", "Galman");
    const QString storedPath = settings.value(m_settingsKey).toString();
    if (!storedPath.isEmpty() && QDir(storedPath).exists() && storedPath != m_rootPath) {
        setRootPath(storedPath);
    }
}

/**
 * @brief Returns the current name filter string.
 * @return Name filter string.
 */
QString FolderBrowserModel::nameFilter() const
{
    return m_nameFilter;
}

/**
 * @brief Sets the name filter and refreshes the model.
 * @param filter Filter string applied to file names.
 */
void FolderBrowserModel::setNameFilter(const QString &filter)
{
    if (filter == m_nameFilter) {
        return;
    }

    m_nameFilter = filter;
    emit nameFilterChanged();
    rebuildEntries();
}

/**
 * @brief Returns the active sort key.
 * @return Sort key enum value.
 */
FolderBrowserModel::SortKey FolderBrowserModel::sortKey() const
{
    return m_sortKey;
}

/**
 * @brief Sets the sort key and refreshes the model.
 * @param key Sort key enum value.
 */
void FolderBrowserModel::setSortKey(FolderBrowserModel::SortKey key)
{
    if (key == m_sortKey) {
        return;
    }

    m_sortKey = key;
    emit sortKeyChanged();
    rebuildEntries();
}

/**
 * @brief Returns the current sort order.
 * @return Sort order value.
 */
Qt::SortOrder FolderBrowserModel::sortOrder() const
{
    return m_sortOrder;
}

/**
 * @brief Sets the sort order and refreshes the model.
 * @param order Sort order value.
 */
void FolderBrowserModel::setSortOrder(Qt::SortOrder order)
{
    if (order == m_sortOrder) {
        return;
    }

    m_sortOrder = order;
    emit sortOrderChanged();
    rebuildEntries();
}

/**
 * @brief Returns whether folders are shown before files.
 * @return True when folders are listed first, false otherwise.
 */
bool FolderBrowserModel::showDirsFirst() const
{
    return m_showDirsFirst;
}

/**
 * @brief Sets whether folders are shown before files and refreshes the model.
 * @param enabled True to list folders first, false otherwise.
 */
void FolderBrowserModel::setShowDirsFirst(bool enabled)
{
    if (enabled == m_showDirsFirst) {
        return;
    }

    m_showDirsFirst = enabled;
    emit showDirsFirstChanged();
    rebuildEntries();
}

/**
 * @brief Returns whether the model is currently loading entries.
 * @return True when loading is in progress, false otherwise.
 */
bool FolderBrowserModel::loading() const
{
    return m_loading;
}

/**
 * @brief Returns the minimum file size filter in bytes.
 * @return Minimum byte size or -1 when unset.
 */
qint64 FolderBrowserModel::minimumByteSize() const
{
    return m_minimumByteSize;
}

/**
 * @brief Sets the minimum file size filter in bytes.
 * @param value Minimum byte size, or -1 to clear.
 */
void FolderBrowserModel::setMinimumByteSize(qint64 value)
{
    const qint64 normalized = value < 0 ? FolderBrowserFilterConstants::unsetByteSize : value;
    if (normalized == m_minimumByteSize) {
        return;
    }
    m_minimumByteSize = normalized;
    emit minimumByteSizeChanged();
    rebuildEntries();
}

/**
 * @brief Returns the maximum file size filter in bytes.
 * @return Maximum byte size or -1 when unset.
 */
qint64 FolderBrowserModel::maximumByteSize() const
{
    return m_maximumByteSize;
}

/**
 * @brief Sets the maximum file size filter in bytes.
 * @param value Maximum byte size, or -1 to clear.
 */
void FolderBrowserModel::setMaximumByteSize(qint64 value)
{
    const qint64 normalized = value < 0 ? FolderBrowserFilterConstants::unsetByteSize : value;
    if (normalized == m_maximumByteSize) {
        return;
    }
    m_maximumByteSize = normalized;
    emit maximumByteSizeChanged();
    rebuildEntries();
}

/**
 * @brief Returns the minimum image width filter in pixels.
 * @return Minimum width or -1 when unset.
 */
int FolderBrowserModel::minimumImageWidth() const
{
    return m_minimumImageWidth;
}

/**
 * @brief Sets the minimum image width filter in pixels.
 * @param value Minimum width, or -1 to clear.
 */
void FolderBrowserModel::setMinimumImageWidth(int value)
{
    const int normalized = value < 0 ? FolderBrowserFilterConstants::unsetDimension : value;
    if (normalized == m_minimumImageWidth) {
        return;
    }
    m_minimumImageWidth = normalized;
    emit minimumImageWidthChanged();
    rebuildEntries();
}

/**
 * @brief Returns the maximum image width filter in pixels.
 * @return Maximum width or -1 when unset.
 */
int FolderBrowserModel::maximumImageWidth() const
{
    return m_maximumImageWidth;
}

/**
 * @brief Sets the maximum image width filter in pixels.
 * @param value Maximum width, or -1 to clear.
 */
void FolderBrowserModel::setMaximumImageWidth(int value)
{
    const int normalized = value < 0 ? FolderBrowserFilterConstants::unsetDimension : value;
    if (normalized == m_maximumImageWidth) {
        return;
    }
    m_maximumImageWidth = normalized;
    emit maximumImageWidthChanged();
    rebuildEntries();
}

/**
 * @brief Returns the minimum image height filter in pixels.
 * @return Minimum height or -1 when unset.
 */
int FolderBrowserModel::minimumImageHeight() const
{
    return m_minimumImageHeight;
}

/**
 * @brief Sets the minimum image height filter in pixels.
 * @param value Minimum height, or -1 to clear.
 */
void FolderBrowserModel::setMinimumImageHeight(int value)
{
    const int normalized = value < 0 ? FolderBrowserFilterConstants::unsetDimension : value;
    if (normalized == m_minimumImageHeight) {
        return;
    }
    m_minimumImageHeight = normalized;
    emit minimumImageHeightChanged();
    rebuildEntries();
}

/**
 * @brief Returns the maximum image height filter in pixels.
 * @return Maximum height or -1 when unset.
 */
int FolderBrowserModel::maximumImageHeight() const
{
    return m_maximumImageHeight;
}

/**
 * @brief Sets the maximum image height filter in pixels.
 * @param value Maximum height, or -1 to clear.
 */
void FolderBrowserModel::setMaximumImageHeight(int value)
{
    const int normalized = value < 0 ? FolderBrowserFilterConstants::unsetDimension : value;
    if (normalized == m_maximumImageHeight) {
        return;
    }
    m_maximumImageHeight = normalized;
    emit maximumImageHeightChanged();
    rebuildEntries();
}

/**
 * @brief Returns the list of selected paths.
 * @return List of selected file or folder paths.
 */
QStringList FolderBrowserModel::selectedPaths() const
{
    return m_selectedPaths;
}

/**
 * @brief Returns whether the selection is a single image.
 * @return True if the selection is one image file, false otherwise.
 */
bool FolderBrowserModel::selectedIsImage() const
{
    return m_selectedIsImage;
}

/**
 * @brief Returns the number of selected files (recursive).
 * @return File count for the current selection.
 */
int FolderBrowserModel::selectedFileCount() const
{
    return m_selectedFileCount;
}

/**
 * @brief Returns the total bytes for selected files (recursive).
 * @return Total byte size of the current selection.
 */
qint64 FolderBrowserModel::selectedTotalBytes() const
{
    return m_selectedTotalBytes;
}

/**
 * @brief Returns whether a copy operation is in progress.
 * @return True when copying is active, false otherwise.
 */
bool FolderBrowserModel::copyInProgress() const
{
    return m_copyInProgress;
}

/**
 * @brief Returns the current copy progress ratio.
 * @return Copy progress in the range 0.0 to 1.0.
 */
qreal FolderBrowserModel::copyProgress() const
{
    return m_copyProgress;
}

/**
 * @brief Returns whether a trash operation is in progress.
 * @return True when trashing is active, false otherwise.
 */
bool FolderBrowserModel::trashInProgress() const
{
    return m_trashInProgress;
}

/**
 * @brief Returns the current trash progress ratio.
 * @return Trash progress in the range 0.0 to 1.0.
 */
qreal FolderBrowserModel::trashProgress() const
{
    return m_trashProgress;
}

/**
 * @brief Returns the number of rows for the model.
 * @param parent Parent index (unused for list model).
 * @return Number of rows.
 */
int FolderBrowserModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_entries.size();
}

/**
 * @brief Returns data for a given model index and role.
 * @param index Model index to read.
 * @param role Data role identifier.
 * @return Role-specific data for the index.
 */
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
    case IsImageRole: {
        if (info.isDir() || !info.exists()) {
            return false;
        }
        return supportedImageFormats().contains(info.suffix().toLower());
    }
    case SuffixRole:
        return info.suffix();
    case CreatedRole: {
        const QDateTime created = info.birthTime().isValid() ? info.birthTime() : info.lastModified();
        return created;
    }
    case ModifiedRole:
        return info.lastModified();
    case SelectedRole:
        return m_selectedPaths.contains(info.absoluteFilePath());
    case CompareStatusRole:
        return 0;
    case GhostRole:
        return false;
    default:
        return {};
    }
}

/**
 * @brief Returns the role names exposed to QML.
 * @return Mapping from role ids to role names.
 */
QHash<int, QByteArray> FolderBrowserModel::roleNames() const
{
    return {
        {FileNameRole, "fileName"},
        {FilePathRole, "filePath"},
        {IsDirRole, "isDir"},
        {IsImageRole, "isImage"},
        {SuffixRole, "suffix"},
        {CreatedRole, "created"},
        {ModifiedRole, "modified"},
        {SelectedRole, "selected"},
        {CompareStatusRole, "compareStatus"},
        {GhostRole, "isGhost"},
    };
}

/**
 * @brief Refreshes entries for the current root path asynchronously.
 */
void FolderBrowserModel::refresh()
{
    if (m_rootPath.isEmpty()) {
        return;
    }

    setLoading(true);
    const int token = ++m_generation;
    const QString path = m_rootPath;

    auto future = QtConcurrent::run([path]() {
        QDir dir(path);
        return dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::NoSort).toVector();
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

/**
 * @brief Activates the entry at the given row.
 * @param row Row index to activate.
 */
void FolderBrowserModel::activate(int row)
{
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

/**
 * @brief Selects or toggles the entry at the given row.
 * @param row Row index to select.
 * @param multi True to toggle selection, false to replace it.
 */
void FolderBrowserModel::select(int row, bool multi)
{
    if (row < 0 || row >= m_entries.size()) {
        return;
    }

    const QString path = m_entries.at(row).absoluteFilePath();
    if (!multi) {
        if (m_selectedPaths.size() == 1 && m_selectedPaths.first() == path) {
            return;
        }
        m_selectedPaths = {path};
        notifySelectionChanged();
        return;
    }

    if (m_selectedPaths.contains(path)) {
        m_selectedPaths.removeAll(path);
    } else {
        m_selectedPaths.append(path);
    }

    notifySelectionChanged();
}

/**
 * @brief Checks whether the entry at the given row is selected.
 * @param row Row index to inspect.
 * @return True if the row is selected, false otherwise.
 */
bool FolderBrowserModel::isSelected(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return false;
    }

    return m_selectedPaths.contains(m_entries.at(row).absoluteFilePath());
}

/**
 * @brief Checks whether the entry at the given row is a folder.
 * @param row Row index to inspect.
 * @return True if the row is a folder, false otherwise.
 */
bool FolderBrowserModel::isDir(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return false;
    }

    return m_entries.at(row).isDir();
}

/**
 * @brief Returns the absolute path for a row.
 * @param row Row index to inspect.
 * @return Absolute path for the row.
 */
QString FolderBrowserModel::pathForRow(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return {};
    }

    return m_entries.at(row).absoluteFilePath();
}

/**
 * @brief Finds the next row whose name starts with a prefix.
 * @param prefix Search prefix (case-insensitive).
 * @param startRow Row index to start searching from.
 * @return Matching row index, or -1 if none found.
 */
int FolderBrowserModel::rowForPrefix(const QString &prefix, int startRow) const
{
    return RowMatchUtils::rowForPrefix(prefix, startRow, m_entries.size(),
        [this](int row) { return m_entries.at(row).fileName(); });
}

/**
 * @brief Formats the last modified time for a row.
 * @param row Row index to inspect.
 * @return Localized short format timestamp string.
 */
QString FolderBrowserModel::modifiedForRow(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return {};
    }

    return QLocale().toString(m_entries.at(row).lastModified(), QLocale::ShortFormat);
}

/**
 * @brief Counts selected items that already exist in a target folder.
 * @param targetDir Target folder path.
 * @return Number of conflicting items.
 */
int FolderBrowserModel::copyNameConflictCount(const QString &targetDir) const
{
    return SelectionStatisticsUtils::countNameConflicts(m_selectedPaths, targetDir);
}

/**
 * @brief Clears the current selection.
 */
void FolderBrowserModel::clearSelection()
{
    if (m_selectedPaths.isEmpty()) {
        return;
    }

    m_selectedPaths.clear();
    notifySelectionChanged();
}

/**
 * @brief Navigates to the parent folder of the current root path.
 */
void FolderBrowserModel::goUp()
{
    if (m_rootPath.isEmpty()) {
        return;
    }

    QDir dir(m_rootPath);
    if (dir.isRoot()) {
        return;
    }

    dir.cdUp();
    setRootPath(dir.absolutePath());
}

/**
 * @brief Refreshes specific files within the current folder.
 * @param paths Absolute paths to refresh.
 */
void FolderBrowserModel::refreshFiles(const QStringList &paths)
{
    if (m_rootPath.isEmpty() || paths.isEmpty()) {
        return;
    }

    const QString rootPath = QDir::cleanPath(m_rootPath);
    const QString parentPrefix = QLatin1String("..");
    const QChar separator = QLatin1Char('/');
    const int invalidIndex = -1;

    QVector<QFileInfo> nextEntries = m_baseEntries;
    bool changed = false;

    for (const QString &path : paths) {
        const QString cleanPath = QDir::cleanPath(path);
        if (cleanPath.isEmpty()) {
            continue;
        }
        if (!cleanPath.startsWith(rootPath)) {
            continue;
        }
        if (cleanPath.size() > rootPath.size()) {
            const QChar boundary = cleanPath.at(rootPath.size());
            if (boundary != separator) {
                continue;
            }
        }

        const QString relativePath = QDir(rootPath).relativeFilePath(cleanPath);
        if (relativePath == parentPrefix || relativePath.startsWith(parentPrefix + separator)) {
            continue;
        }

        const QFileInfo info(cleanPath);
        int existingIndex = invalidIndex;
        for (int i = 0; i < nextEntries.size(); i += 1) {
            if (nextEntries.at(i).absoluteFilePath() == cleanPath) {
                existingIndex = i;
                break;
            }
        }

        if (info.exists()) {
            if (existingIndex == invalidIndex) {
                nextEntries.append(info);
            } else {
                nextEntries[existingIndex] = info;
            }
            changed = true;
        } else if (existingIndex != invalidIndex) {
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

/**
 * @brief Sets the selection from a list of row indices.
 * @param rows List of row indices to select.
 * @param additive True to add to current selection, false to replace it.
 */
void FolderBrowserModel::setSelection(const QVariantList &rows, bool additive)
{
    QStringList next;
    if (additive) {
        next = m_selectedPaths;
    }

    for (const QVariant &value : rows) {
        const int row = value.toInt();
        if (row < 0 || row >= m_entries.size()) {
            continue;
        }
        const QString path = m_entries.at(row).absoluteFilePath();
        if (!next.contains(path)) {
            next.append(path);
        }
    }

    if (next == m_selectedPaths) {
        return;
    }

    m_selectedPaths = next;
    notifySelectionChanged();
}

/**
 * @brief Sets the selection for a continuous row range.
 * @param start Start row index.
 * @param end End row index.
 * @param additive True to add to current selection, false to replace it.
 */
void FolderBrowserModel::setSelectionRange(int start, int end, bool additive)
{
    if (m_entries.isEmpty()) {
        return;
    }

    int from = std::min(start, end);
    int to = std::max(start, end);
    from = std::max(0, from);
    to = std::min(to, static_cast<int>(m_entries.size()) - 1);

    QVariantList rows;
    rows.reserve(to - from + 1);
    for (int i = from; i <= to; ++i) {
        rows.append(i);
    }

    setSelection(rows, additive);
}

/**
 * @brief Checks whether all entries are selected.
 * @return True if all entries are selected, false otherwise.
 */
bool FolderBrowserModel::allSelected() const
{
    if (m_entries.isEmpty()) {
        return false;
    }

    return m_selectedPaths.size() == m_entries.size();
}

/**
 * @brief Returns the row indices of selected entries.
 * @return List of selected row indices.
 */
QVariantList FolderBrowserModel::selectedRows() const
{
    QVariantList rows;
    if (m_entries.isEmpty() || m_selectedPaths.isEmpty()) {
        return rows;
    }
    rows.reserve(m_selectedPaths.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        const QString path = m_entries.at(i).absoluteFilePath();
        if (m_selectedPaths.contains(path)) {
            rows.append(i);
        }
    }
    return rows;
}

/**
 * @brief Checks whether the entry at the given row is an image file.
 * @param row Row index to inspect.
 * @return True if the row is an image file, false otherwise.
 */
bool FolderBrowserModel::isImage(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return false;
    }
    return isImagePath(m_entries.at(row).absoluteFilePath());
}

/**
 * @brief Returns the compare status for the current selection.
 * @return Compare status value.
 */
int FolderBrowserModel::selectedCompareStatus() const
{
    return 0;
}

/**
 * @brief Returns whether the selection is marked as ghost.
 * @return True if selection is ghost, false otherwise.
 */
bool FolderBrowserModel::selectedIsGhost() const
{
    return false;
}

/**
 * @brief Computes counts of selected folders and files.
 * @return Map with "dirs" and "files" counts.
 */
QVariantMap FolderBrowserModel::selectionStats() const
{
    QVariantMap result;
    const SelectionStatisticsUtils::SelectionStatisticsResult stats
        = SelectionStatisticsUtils::computeStatistics(m_selectedPaths);
    result.insert("dirs", stats.folderCount);
    result.insert("files", stats.fileCount);
    return result;
}

/**
 * @brief Copies selected items to a target folder synchronously.
 * @param targetDir Target folder path.
 * @return Result map including ok, copied, failed, and error fields.
 */
QVariantMap FolderBrowserModel::copySelectedTo(const QString &targetDir)
{
    QVariantMap result;
    result.insert("ok", false);

    if (targetDir.isEmpty() || !QDir(targetDir).exists()) {
        result.insert("error", tr("Target folder not found"));
        return result;
    }

    const int total = m_selectedPaths.size();
    int processed = 0;
    setCopyInProgress(true);
    updateCopyProgress(0, total);

    int copied = 0;
    int failed = 0;
    QString firstError;
    const QDir dir(targetDir);
    auto finishItem = [&]() {
        processed += 1;
        updateCopyProgress(processed, total);
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    };

    for (const QString &path : m_selectedPaths) {
        QFileInfo info(path);
        if (!info.exists()) {
            failed += 1;
            if (firstError.isEmpty()) {
                firstError = tr("Source not found");
            }
            finishItem();
            continue;
        }
        const QString targetPath = dir.filePath(info.fileName());
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
            if (!FileOperationUtils::copyFolderRecursive(path, targetPath, "FolderBrowserModel", &error)) {
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
            const QFileInfo sourceInfo(path);
            FileOperationUtils::applyFileTimes(sourceInfo, targetPath);
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

void FolderBrowserModel::startTransferSelectedTo(const QString &targetDir, bool moveItems)
{
    if (m_copyInProgress) {
        return;
    }
    if (targetDir.isEmpty() || !QDir(targetDir).exists()) {
        QVariantMap result;
        result.insert("ok", false);
        result.insert("error", tr("Target folder not found"));
        emit copyFinished(result);
        return;
    }
    if (m_selectedPaths.isEmpty()) {
        QVariantMap result;
        result.insert("ok", false);
        result.insert("error", tr("No items selected"));
        emit copyFinished(result);
        return;
    }

    QList<CopyItem> items;
    items.reserve(m_selectedPaths.size());
    const QDir dir(targetDir);
    for (const QString &path : m_selectedPaths) {
        const QFileInfo info(path);
        CopyItem item;
        item.sourcePath = path;
        item.targetPath = dir.filePath(info.fileName());
        item.isDir = info.isDir();
        items.append(item);
    }

    const CopyWorker::OperationMode mode = moveItems
        ? CopyWorker::OperationMode::Move
        : CopyWorker::OperationMode::Copy;
    auto *thread = new QThread(this);
    auto *worker = new CopyWorker(items, mode);
    worker->moveToThread(thread);

    m_copyThread = thread;
    m_copyWorker = worker;
    setCopyInProgress(true);
    updateCopyProgress(0, 0);

    connect(thread, &QThread::started, worker, &CopyWorker::start);
    connect(worker, &CopyWorker::progress, this, &FolderBrowserModel::updateCopyProgress);
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

/**
 * @brief Starts an asynchronous copy of selected items to a target folder.
 * @param targetDir Target folder path.
 */
void FolderBrowserModel::startCopySelectedTo(const QString &targetDir)
{
    startTransferSelectedTo(targetDir, false);
}

/**
 * @brief Starts an asynchronous move of selected items to a target folder.
 * @param targetDir Target folder path.
 */
void FolderBrowserModel::startMoveSelectedTo(const QString &targetDir)
{
    startTransferSelectedTo(targetDir, true);
}

/**
 * @brief Requests cancellation of the current copy operation.
 */
void FolderBrowserModel::cancelCopy()
{
    if (!m_copyWorker) {
        return;
    }
    QMetaObject::invokeMethod(m_copyWorker, "cancel", Qt::QueuedConnection);
}

/**
 * @brief Starts an asynchronous move of selected items to trash.
 * @return Result map indicating the operation was started.
 */
QVariantMap FolderBrowserModel::moveSelectedToTrash()
{
    return requestRemoval(true);
}

/**
 * @brief Starts an asynchronous trash operation for selected items.
 */
void FolderBrowserModel::startMoveSelectedToTrash()
{
    startRemoval(true);
}

/**
 * @brief Starts an asynchronous permanent delete for selected items.
 * @return Result map indicating the operation was started.
 */
QVariantMap FolderBrowserModel::deleteSelectedPermanently()
{
    return requestRemoval(false);
}

/**
 * @brief Starts an asynchronous permanent delete operation for selected items.
 */
void FolderBrowserModel::startDeleteSelectedPermanently()
{
    startRemoval(false);
}

/**
 * @brief Starts an asynchronous removal for selected items.
 * @param moveToTrash True to move to trash when supported, false to delete permanently.
 * @return Result map indicating the operation was started.
 */
QVariantMap FolderBrowserModel::requestRemoval(bool moveToTrash)
{
    QVariantMap result;
    const bool hasSelection = !m_selectedPaths.isEmpty();
    if (!hasSelection) {
        result.insert("ok", false);
        result.insert("error", tr("Nothing to delete"));
        return result;
    }
    startRemoval(moveToTrash);
    result.insert("ok", true);
    result.insert("pending", FolderBrowserTrashConstants::pendingTrue);
    return result;
}

/**
 * @brief Starts an asynchronous removal operation for selected items.
 * @param moveToTrash True to move to trash when supported, false to delete permanently.
 */
void FolderBrowserModel::startRemoval(bool moveToTrash)
{
    if (m_trashInProgress) {
        return;
    }
    const QStringList paths = m_selectedPaths;
    if (paths.isEmpty()) {
        QVariantMap result;
        result.insert("ok", false);
        result.insert("error", tr("Nothing to delete"));
        emit trashFinished(result);
        return;
    }

    auto *thread = new QThread(this);
    const TrashWorker::RemovalMode mode = moveToTrash ? TrashWorker::MoveToTrash : TrashWorker::DeletePermanently;
    auto *worker = new TrashWorker(paths, mode);
    worker->moveToThread(thread);

    m_trashThread = thread;
    m_trashWorker = worker;
    setTrashInProgress(true);
    updateTrashProgress(FolderBrowserTrashConstants::emptyCount, paths.size());

    connect(thread, &QThread::started, worker, &TrashWorker::start);
    connect(worker, &TrashWorker::progress, this, &FolderBrowserModel::updateTrashProgress);
    connect(worker, &TrashWorker::finished, this, [this, thread](const QVariantMap &result) {
        setTrashInProgress(false);
        updateTrashProgress(FolderBrowserTrashConstants::emptyCount, FolderBrowserTrashConstants::emptyCount);
        emit trashFinished(result);

        const int moved = result.value("moved").toInt();
        if (moved > FolderBrowserTrashConstants::emptyCount) {
            refresh();
        }
        clearSelection();

        m_trashWorker = nullptr;
        m_trashThread = nullptr;
        thread->quit();
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

/**
 * @brief Requests cancellation of the current trash operation.
 */
void FolderBrowserModel::cancelTrash()
{
    if (!m_trashWorker) {
        return;
    }
    QMetaObject::invokeMethod(m_trashWorker, "cancel", Qt::QueuedConnection);
}

/**
 * @brief Renames a file or folder within the current folder.
 * @param path Full path to rename.
 * @param newName New file or folder name (without path).
 * @return Result map including ok, newPath, and error fields.
 */
QVariantMap FolderBrowserModel::renamePath(const QString &path, const QString &newName)
{
    QVariantMap result;
    result.insert("ok", false);
    QString targetPath;
    QString error;
    if (!PlatformUtils::renamePath(path, newName, &targetPath, &error)) {
        result.insert("error", error.isEmpty() ? tr("Rename failed") : error);
        return result;
    }

    QStringList nextSelection = m_selectedPaths;
    const int selectedIndex = nextSelection.indexOf(path);
    if (selectedIndex >= 0) {
        nextSelection[selectedIndex] = targetPath;
    }
    if (nextSelection != m_selectedPaths) {
        m_selectedPaths = nextSelection;
        notifySelectionChanged();
    }

    refreshFiles({path, targetPath});
    result.insert("ok", true);
    result.insert("newPath", targetPath);
    return result;
}

/**
 * @brief Updates the loading state and emits change notification.
 * @param loading True when loading is active, false otherwise.
 */
void FolderBrowserModel::setLoading(bool loading)
{
    if (loading == m_loading) {
        return;
    }

    m_loading = loading;
    emit loadingChanged();
}

/**
 * @brief Updates the copy-in-progress state and schedules refresh if needed.
 * @param inProgress True when copy is active, false otherwise.
 */
void FolderBrowserModel::setCopyInProgress(bool inProgress)
{
    if (m_copyInProgress == inProgress) {
        return;
    }
    m_copyInProgress = inProgress;
    emit copyInProgressChanged();
    if (!m_copyInProgress && !m_trashInProgress && m_pendingWatcherRefresh) {
        m_pendingWatcherRefresh = false;
        scheduleRefresh();
    }
}

/**
 * @brief Updates copy counters and emits progress when it changes.
 * @param completed Number of completed entries.
 * @param total Total number of entries to copy.
 */
void FolderBrowserModel::updateCopyProgress(int completed, int total)
{
    m_copyCompleted = completed;
    m_copyTotal = total;
    const qreal nextProgress = total > 0
        ? static_cast<qreal>(completed) / static_cast<qreal>(total)
        : FolderBrowserTrashConstants::zeroProgress;
    if (!qFuzzyCompare(m_copyProgress, nextProgress)) {
        m_copyProgress = nextProgress;
        emit copyProgressChanged();
    }
}

/**
 * @brief Updates the trash-in-progress state and schedules refresh if needed.
 * @param inProgress True when trash is active, false otherwise.
 */
void FolderBrowserModel::setTrashInProgress(bool inProgress)
{
    if (m_trashInProgress == inProgress) {
        return;
    }
    m_trashInProgress = inProgress;
    emit trashInProgressChanged();
    if (!m_trashInProgress && !m_copyInProgress && m_pendingWatcherRefresh) {
        m_pendingWatcherRefresh = false;
        scheduleRefresh();
    }
}

/**
 * @brief Updates trash counters and emits progress when it changes.
 * @param completed Number of completed entries.
 * @param total Total number of entries to trash.
 */
void FolderBrowserModel::updateTrashProgress(int completed, int total)
{
    m_trashCompleted = completed;
    m_trashTotal = total;
    const qreal nextProgress = total > FolderBrowserTrashConstants::emptyCount
        ? static_cast<qreal>(completed) / static_cast<qreal>(total)
        : FolderBrowserTrashConstants::zeroProgress;
    if (!qFuzzyCompare(m_trashProgress, nextProgress)) {
        m_trashProgress = nextProgress;
        emit trashProgressChanged();
    }
}

/**
 * @brief Schedules a delayed refresh to coalesce rapid updates.
 */
void FolderBrowserModel::scheduleRefresh()
{
    if (!m_refreshTimer.isActive()) {
        m_refreshTimer.start();
    }
}

/**
 * @brief Updates file watchers to observe current file entries.
 * @param entries Current file entries for the root folder.
 */
void FolderBrowserModel::updateFileWatchers(const QVector<QFileInfo> &entries)
{
    QStringList nextFiles;
    nextFiles.reserve(entries.size());
    for (const QFileInfo &info : entries) {
        if (info.isFile()) {
            nextFiles.append(info.absoluteFilePath());
        }
    }

    const QStringList currentFiles = m_watcher.files();
    if (!currentFiles.isEmpty()) {
        m_watcher.removePaths(currentFiles);
    }
    if (!nextFiles.isEmpty()) {
        Q_UNUSED(m_watcher.addPaths(nextFiles));
    }
}

/**
 * @brief Applies an incremental update of entries to the model.
 * @param entries New entries to apply.
 */
void FolderBrowserModel::applyEntriesIncremental(const QVector<QFileInfo> &entries)
{
    const QVector<int> dataRoles = {
        FileNameRole,
        FilePathRole,
        IsDirRole,
        IsImageRole,
        SuffixRole,
        CreatedRole,
        ModifiedRole,
        CompareStatusRole,
        GhostRole,
    };

    int i = 0;
    while (i < entries.size()) {
        const QString nextPath = entries.at(i).absoluteFilePath();
        if (i < m_entries.size() && m_entries.at(i).absoluteFilePath() == nextPath) {
            if (!fileInfoEquivalent(m_entries.at(i), entries.at(i))) {
                m_entries[i] = entries.at(i);
                emit dataChanged(index(i, 0), index(i, 0), dataRoles);
            } else {
                m_entries[i] = entries.at(i);
            }
            i += 1;
            continue;
        }

        int existing = -1;
        for (int j = i + 1; j < m_entries.size(); j += 1) {
            if (m_entries.at(j).absoluteFilePath() == nextPath) {
                existing = j;
                break;
            }
        }

        if (existing >= 0) {
            beginMoveRows(QModelIndex(), existing, existing, QModelIndex(), i);
            const QFileInfo moved = m_entries.takeAt(existing);
            m_entries.insert(i, moved);
            endMoveRows();
            if (!fileInfoEquivalent(m_entries.at(i), entries.at(i))) {
                m_entries[i] = entries.at(i);
                emit dataChanged(index(i, 0), index(i, 0), dataRoles);
            } else {
                m_entries[i] = entries.at(i);
            }
            i += 1;
            continue;
        }

        beginInsertRows(QModelIndex(), i, i);
        m_entries.insert(i, entries.at(i));
        endInsertRows();
        i += 1;
    }

    if (m_entries.size() > entries.size()) {
        beginRemoveRows(QModelIndex(), entries.size(), m_entries.size() - 1);
        m_entries.remove(entries.size(), m_entries.size() - entries.size());
        endRemoveRows();
    }
}

/**
 * @brief Returns whether byte size filters are active.
 * @return True when any byte size filter is set.
 */
bool FolderBrowserModel::byteSizeFiltersActive() const
{
    return m_minimumByteSize > FolderBrowserFilterConstants::unsetByteSize
        || m_maximumByteSize > FolderBrowserFilterConstants::unsetByteSize;
}

/**
 * @brief Returns whether image size filters are active.
 * @return True when any image dimension filter is set.
 */
bool FolderBrowserModel::imageSizeFiltersActive() const
{
    return m_minimumImageWidth > FolderBrowserFilterConstants::unsetDimension
        || m_maximumImageWidth > FolderBrowserFilterConstants::unsetDimension
        || m_minimumImageHeight > FolderBrowserFilterConstants::unsetDimension
        || m_maximumImageHeight > FolderBrowserFilterConstants::unsetDimension;
}

/**
 * @brief Returns whether signature-based sorting is active.
 * @return True when sorting by signature.
 */
bool FolderBrowserModel::signatureSortActive() const
{
    return m_sortKey == Signature;
}

/**
 * @brief Removes cached entries that are no longer present.
 * @param entries Current base entries to keep.
 */
void FolderBrowserModel::pruneCaches(const QVector<QFileInfo> &entries)
{
    QSet<QString> currentPaths;
    currentPaths.reserve(entries.size());
    for (const QFileInfo &info : entries) {
        currentPaths.insert(info.absoluteFilePath());
    }

    for (auto it = m_imageSizeCache.begin(); it != m_imageSizeCache.end();) {
        if (!currentPaths.contains(it.key())) {
            it = m_imageSizeCache.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_imageSizeAttempted.begin(); it != m_imageSizeAttempted.end();) {
        if (!currentPaths.contains(*it)) {
            it = m_imageSizeAttempted.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_signatureHashCache.begin(); it != m_signatureHashCache.end();) {
        if (!currentPaths.contains(it.key())) {
            it = m_signatureHashCache.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_signatureHashAttempted.begin(); it != m_signatureHashAttempted.end();) {
        if (!currentPaths.contains(*it)) {
            it = m_signatureHashAttempted.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * @brief Rebuilds entries using current filters and sorting.
 */
void FolderBrowserModel::rebuildEntries()
{
    if (imageSizeFiltersActive()) {
        requestImageSizeRefresh();
    }
    if (signatureSortActive()) {
        requestSignatureHashRefresh();
    }

    QVector<QFileInfo> filtered = m_baseEntries;
    applyFilterAndSort(filtered);
    applyEntriesIncremental(filtered);

    QStringList nextSelection;
    nextSelection.reserve(m_selectedPaths.size());
    for (const QString &path : m_selectedPaths) {
        const QFileInfo info(path);
        if (info.exists()) {
            nextSelection.append(path);
        }
    }
    if (nextSelection != m_selectedPaths) {
        m_selectedPaths = nextSelection;
        notifySelectionChanged();
    }
}

/**
 * @brief Starts asynchronous image size reads when needed.
 */
void FolderBrowserModel::requestImageSizeRefresh()
{
    if (m_imageSizeLoading) {
        return;
    }
    if (!imageSizeFiltersActive()) {
        return;
    }

    QStringList pending;
    pending.reserve(m_baseEntries.size());
    for (const QFileInfo &info : m_baseEntries) {
        if (info.isDir()) {
            continue;
        }
        if (!isImageInfo(info)) {
            continue;
        }
        const QString path = info.absoluteFilePath();
        if (m_imageSizeAttempted.contains(path)) {
            continue;
        }
        pending.append(path);
    }

    if (pending.isEmpty()) {
        return;
    }

    m_imageSizeLoading = true;
    const int token = ++m_imageSizeGeneration;

    auto future = QtConcurrent::run([pending]() {
        return ImageMetadataUtils::readImageSizes(pending);
    });

    auto *watcher = new QFutureWatcher<ImageMetadataUtils::ImageSizeBatchResult>(this);
    connect(watcher, &QFutureWatcher<ImageMetadataUtils::ImageSizeBatchResult>::finished, this, [this, watcher, token]() {
        const auto result = watcher->result();
        watcher->deleteLater();

        if (token != m_imageSizeGeneration) {
            return;
        }

        for (auto it = result.sizes.constBegin(); it != result.sizes.constEnd(); ++it) {
            m_imageSizeCache.insert(it.key(), it.value());
        }
        for (const QString &path : result.attempted) {
            m_imageSizeAttempted.insert(path);
        }
        m_imageSizeLoading = false;
        rebuildEntries();
    });

    watcher->setFuture(future);
}

/**
 * @brief Starts asynchronous signature hash reads when needed.
 */
void FolderBrowserModel::requestSignatureHashRefresh()
{
    if (m_signatureHashLoading) {
        return;
    }
    if (!signatureSortActive()) {
        return;
    }

    QStringList pending;
    pending.reserve(m_baseEntries.size());
    for (const QFileInfo &info : m_baseEntries) {
        if (info.isDir()) {
            continue;
        }
        if (!isImageInfo(info)) {
            continue;
        }
        const QString path = info.absoluteFilePath();
        if (m_signatureHashAttempted.contains(path)) {
            continue;
        }
        pending.append(path);
    }

    if (pending.isEmpty()) {
        return;
    }

    m_signatureHashLoading = true;
    const int token = ++m_signatureHashGeneration;

    auto future = QtConcurrent::run([pending]() {
        return ImageMetadataUtils::readSignatureHashes(pending, FolderBrowserFilterConstants::signatureDimension);
    });

    auto *watcher = new QFutureWatcher<ImageMetadataUtils::SignatureHashBatchResult>(this);
    connect(watcher, &QFutureWatcher<ImageMetadataUtils::SignatureHashBatchResult>::finished, this, [this, watcher, token]() {
        const auto result = watcher->result();
        watcher->deleteLater();

        if (token != m_signatureHashGeneration) {
            return;
        }

        for (auto it = result.hashes.constBegin(); it != result.hashes.constEnd(); ++it) {
            m_signatureHashCache.insert(it.key(), it.value());
        }
        for (const QString &path : result.attempted) {
            m_signatureHashAttempted.insert(path);
        }
        m_signatureHashLoading = false;
        rebuildEntries();
    });

    watcher->setFuture(future);
}

/**
 * @brief Filters and sorts entries based on the current settings.
 * @param entries Entries to filter and sort in place.
 */
void FolderBrowserModel::applyFilterAndSort(QVector<QFileInfo> &entries) const
{
    const QString trimmed = m_nameFilter.trimmed();
    const bool nameFilterActive = !trimmed.isEmpty();
    const bool byteSizeActive = byteSizeFiltersActive();
    const bool imageSizeActive = imageSizeFiltersActive();
    const QString needle = trimmed.toLower();

    if (nameFilterActive || byteSizeActive || imageSizeActive) {
        entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const QFileInfo &info) {
            if (nameFilterActive && !info.fileName().toLower().contains(needle)) {
                return true;
            }

            if (byteSizeActive && !info.isDir()) {
                const qint64 byteSize = info.size();
                if (m_minimumByteSize > FolderBrowserFilterConstants::unsetByteSize
                    && byteSize < m_minimumByteSize) {
                    return true;
                }
                if (m_maximumByteSize > FolderBrowserFilterConstants::unsetByteSize
                    && byteSize > m_maximumByteSize) {
                    return true;
                }
            }

            if (imageSizeActive) {
                if (!isImageInfo(info)) {
                    return true;
                }
                const QString path = info.absoluteFilePath();
                const bool hasSize = m_imageSizeCache.contains(path);
                if (!hasSize) {
                    return true;
                }
                const QSize size = m_imageSizeCache.value(path);
                if (!size.isValid()) {
                    return true;
                }
                if (m_minimumImageWidth > FolderBrowserFilterConstants::unsetDimension
                    && size.width() < m_minimumImageWidth) {
                    return true;
                }
                if (m_maximumImageWidth > FolderBrowserFilterConstants::unsetDimension
                    && size.width() > m_maximumImageWidth) {
                    return true;
                }
                if (m_minimumImageHeight > FolderBrowserFilterConstants::unsetDimension
                    && size.height() < m_minimumImageHeight) {
                    return true;
                }
                if (m_maximumImageHeight > FolderBrowserFilterConstants::unsetDimension
                    && size.height() > m_maximumImageHeight) {
                    return true;
                }
            }

            return false;
        }), entries.end());
    }

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    auto compare = [&](const QFileInfo &left, const QFileInfo &right) {
        if (m_showDirsFirst && left.isDir() != right.isDir()) {
            return left.isDir();
        }

        bool result = false;
        switch (m_sortKey) {
        case Extension:
            result = collator.compare(left.suffix(), right.suffix()) < 0;
            break;
        case Created: {
            const QDateTime leftTime = left.birthTime().isValid() ? left.birthTime() : left.lastModified();
            const QDateTime rightTime = right.birthTime().isValid() ? right.birthTime() : right.lastModified();
            result = leftTime < rightTime;
            break;
        }
        case Modified:
            result = left.lastModified() < right.lastModified();
            break;
        case Signature: {
            if (!left.isDir() && !right.isDir()) {
                const QString leftPath = left.absoluteFilePath();
                const QString rightPath = right.absoluteFilePath();
                const bool leftHasSignature = m_signatureHashCache.contains(leftPath);
                const bool rightHasSignature = m_signatureHashCache.contains(rightPath);
                if (leftHasSignature && rightHasSignature) {
                    result = m_signatureHashCache.value(leftPath) < m_signatureHashCache.value(rightPath);
                } else if (leftHasSignature != rightHasSignature) {
                    result = leftHasSignature;
                } else {
                    result = collator.compare(left.fileName(), right.fileName()) < 0;
                }
            } else {
                result = collator.compare(left.fileName(), right.fileName()) < 0;
            }
            break;
        }
        case Name:
        default:
            result = collator.compare(left.fileName(), right.fileName()) < 0;
            break;
        }

        return m_sortOrder == Qt::AscendingOrder ? result : !result;
    };

    std::sort(entries.begin(), entries.end(), compare);
}

/**
 * @brief Emits selection change signals and updates selection-dependent roles.
 */
void FolderBrowserModel::notifySelectionChanged()
{
    emit selectedPathsChanged();
    const bool nextIsImage = m_selectedPaths.size() == 1 && isImagePath(m_selectedPaths.first());
    if (nextIsImage != m_selectedIsImage) {
        m_selectedIsImage = nextIsImage;
        emit selectedIsImageChanged();
    }
    updateSelectionTotalsAsync();

    if (m_entries.isEmpty()) {
        return;
    }

    const QModelIndex first = index(0, 0);
    const QModelIndex last = index(m_entries.size() - 1, 0);
    emit dataChanged(first, last, {SelectedRole});
}

/**
 * @brief Updates cached selection totals asynchronously.
 */
void FolderBrowserModel::updateSelectionTotalsAsync()
{
    SelectionStatisticsUtils::updateSelectionTotalsAsync(this,
        m_selectedPaths,
        &m_selectionTotalsGeneration,
        [this](int fileCount, qint64 totalBytes) {
            setSelectionTotals(fileCount, totalBytes);
        });
}

/**
 * @brief Sets selection totals and emits change signals if needed.
 * @param fileCount Selected file count.
 * @param totalBytes Selected total byte size.
 */
void FolderBrowserModel::setSelectionTotals(int fileCount, qint64 totalBytes)
{
    bool countChanged = false;
    bool bytesChanged = false;
    if (m_selectedFileCount != fileCount) {
        m_selectedFileCount = fileCount;
        countChanged = true;
    }
    if (m_selectedTotalBytes != totalBytes) {
        m_selectedTotalBytes = totalBytes;
        bytesChanged = true;
    }
    if (countChanged) {
        emit selectedFileCountChanged();
    }
    if (bytesChanged) {
        emit selectedTotalBytesChanged();
    }
}
