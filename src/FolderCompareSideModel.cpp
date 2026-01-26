
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

#include <QCollator>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMetaObject>
#include <QIODevice>
#include <QSettings>
#include <QThread>
#include <algorithm>

#include "CopyWorker.h"
#include "FolderCompareModel.h"
#include "PlatformUtils.h"
#include "TrashWorker.h"

namespace {

/**
 * @brief Applies source timestamps to the target path.
 * @param sourceInfo Source file information to copy timestamps from.
 * @param targetPath Destination file path to update.
 */
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

struct FolderCompareTrashConstants {
    static constexpr int emptyCount = 0;
    static constexpr qreal zeroProgress = 0.0;
    static constexpr bool pendingTrue = true;
};

/**
 * @brief Copies a folder recursively from source to target.
 * @param sourcePath Source folder path.
 * @param targetPath Target folder path.
 * @param error Optional output error message.
 * @return True when the copy succeeds, false otherwise.
 */
bool copyDirectoryRecursive(const QString &sourcePath, const QString &targetPath, QString *error)
{
    const QDir sourceDir(sourcePath);
    if (!sourceDir.exists()) {
        if (error) {
            *error = QCoreApplication::translate("FolderCompareSideModel", "Source not found");
        }
        return false;
    }
    if (QFileInfo::exists(targetPath)) {
        if (error) {
            *error = QCoreApplication::translate("FolderCompareSideModel", "Target already exists");
        }
        return false;
    }
    if (!QDir().mkpath(targetPath)) {
        if (error) {
            *error = QCoreApplication::translate("FolderCompareSideModel", "Cannot create target folder");
        }
        return false;
    }

    const QFileInfoList entries = sourceDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::NoSort);
    for (const QFileInfo &entry : entries) {
        const QString entryPath = entry.absoluteFilePath();
        const QString targetEntryPath = QDir(targetPath).filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyDirectoryRecursive(entryPath, targetEntryPath, error)) {
                return false;
            }
        } else {
            if (QFileInfo::exists(targetEntryPath)) {
                if (error) {
                    *error = QCoreApplication::translate("FolderCompareSideModel", "Target already exists");
                }
                return false;
            }
            if (!QFile::copy(entryPath, targetEntryPath)) {
                if (error) {
                    *error = QCoreApplication::translate("FolderCompareSideModel", "Copy failed");
                }
                return false;
            }
            applyFileTimes(entry, targetEntryPath);
        }
    }
    return true;
}

/**
 * @brief Counts folders and files under a path recursively.
 * @param path Root path to count.
 * @param dirCount Reference to folder counter to update.
 * @param fileCount Reference to file counter to update.
 */
void countPathRecursive(const QString &path, int &dirCount, int &fileCount)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        return;
    }
    if (info.isDir()) {
        dirCount += 1;
        const QDir dir(path);
        const QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::NoSort);
        for (const QFileInfo &entry : entries) {
            countPathRecursive(entry.absoluteFilePath(), dirCount, fileCount);
        }
    } else {
        fileCount += 1;
    }
}

/**
 * @brief Compares two compare entries for equality.
 * @param left First entry to compare.
 * @param right Second entry to compare.
 * @return True when entries are equivalent, false otherwise.
 */
bool compareEntryEquivalent(const FolderCompareSideModel::CompareEntry &left,
                            const FolderCompareSideModel::CompareEntry &right)
{
    return left.id == right.id
        && left.fileName == right.fileName
        && left.filePath == right.filePath
        && left.created == right.created
        && left.modified == right.modified
        && left.isDir == right.isDir
        && left.isImage == right.isImage
        && left.isGhost == right.isGhost
        && left.status == right.status;
}

} // namespace

/**
 * @brief Constructs a side model bound to a compare model and side index.
 * @param compareModel Parent compare model instance.
 * @param side Side index for left or right.
 * @param parent Parent QObject for ownership.
 */
FolderCompareSideModel::FolderCompareSideModel(FolderCompareModel *compareModel, int side, QObject *parent)
    : QAbstractListModel(parent)
    , m_compareModel(compareModel)
    , m_side(side)
{
    if (m_compareModel) {
        connect(m_compareModel, &FolderCompareModel::loadingChanged, this, [this]() {
            setLoading(m_compareModel->loading());
        });
    }
}

/**
 * @brief Returns the current root path for this side.
 * @return Root folder path.
 */
QString FolderCompareSideModel::rootPath() const
{
    return m_rootPath;
}

/**
 * @brief Sets the root path for this side and refreshes entries.
 * @param path Root folder path to set.
 */
void FolderCompareSideModel::setRootPath(const QString &path)
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

    if (m_compareModel) {
        m_compareModel->setSidePath(static_cast<FolderCompareModel::Side>(m_side), m_rootPath);
    }

    refresh();
    clearSelection();
}

/**
 * @brief Returns the settings key used to persist the root path.
 * @return Settings key string.
 */
QString FolderCompareSideModel::settingsKey() const
{
    return m_settingsKey;
}

/**
 * @brief Sets the settings key and restores a stored root path if present.
 * @param key Settings key string to use for persistence.
 */
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
QString FolderCompareSideModel::nameFilter() const
{
    return m_nameFilter;
}

/**
 * @brief Sets the name filter and rebuilds entries.
 * @param filter Filter string applied to file names.
 */
void FolderCompareSideModel::setNameFilter(const QString &filter)
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
FolderCompareSideModel::SortKey FolderCompareSideModel::sortKey() const
{
    return m_sortKey;
}

/**
 * @brief Sets the sort key and rebuilds entries.
 * @param key Sort key enum value.
 */
void FolderCompareSideModel::setSortKey(FolderCompareSideModel::SortKey key)
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
Qt::SortOrder FolderCompareSideModel::sortOrder() const
{
    return m_sortOrder;
}

/**
 * @brief Sets the sort order and rebuilds entries.
 * @param order Sort order value.
 */
void FolderCompareSideModel::setSortOrder(Qt::SortOrder order)
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
bool FolderCompareSideModel::showDirsFirst() const
{
    return m_showDirsFirst;
}

/**
 * @brief Sets whether folders are shown before files and rebuilds entries.
 * @param enabled True to list folders first, false otherwise.
 */
void FolderCompareSideModel::setShowDirsFirst(bool enabled)
{
    if (enabled == m_showDirsFirst) {
        return;
    }
    m_showDirsFirst = enabled;
    emit showDirsFirstChanged();
    rebuildEntries();
}

/**
 * @brief Returns whether the compare model is loading entries.
 * @return True when loading is in progress, false otherwise.
 */
bool FolderCompareSideModel::loading() const
{
    return m_loading;
}

/**
 * @brief Sets whether identical items are hidden.
 * @param hide True to hide identical items, false otherwise.
 */
void FolderCompareSideModel::setHideIdentical(bool hide)
{
    if (m_hideIdentical == hide) {
        return;
    }
    m_hideIdentical = hide;
    rebuildEntries();
}

/**
 * @brief Returns the list of selected paths for this side.
 * @return List of selected file or folder paths.
 */
QStringList FolderCompareSideModel::selectedPaths() const
{
    return m_selectedPaths;
}

/**
 * @brief Returns whether the selection is a single image.
 * @return True if the selection is one image file, false otherwise.
 */
bool FolderCompareSideModel::selectedIsImage() const
{
    return m_selectedIsImage;
}

/**
 * @brief Returns whether a copy operation is in progress.
 * @return True when copying is active, false otherwise.
 */
bool FolderCompareSideModel::copyInProgress() const
{
    return m_copyInProgress;
}

/**
 * @brief Returns the current copy progress ratio.
 * @return Copy progress in the range 0.0 to 1.0.
 */
qreal FolderCompareSideModel::copyProgress() const
{
    return m_copyProgress;
}

/**
 * @brief Returns whether a trash operation is in progress.
 * @return True when trashing is active, false otherwise.
 */
bool FolderCompareSideModel::trashInProgress() const
{
    return m_trashInProgress;
}

/**
 * @brief Returns the current trash progress ratio.
 * @return Trash progress in the range 0.0 to 1.0.
 */
qreal FolderCompareSideModel::trashProgress() const
{
    return m_trashProgress;
}

/**
 * @brief Returns the number of rows for the model.
 * @param parent Parent index (unused for list model).
 * @return Number of rows.
 */
int FolderCompareSideModel::rowCount(const QModelIndex &parent) const
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
    case IsDirRole:
        return entry.isDir;
    case IsImageRole:
        return entry.isImage;
    case SuffixRole: {
        const int dot = entry.fileName.lastIndexOf('.');
        return dot >= 0 ? entry.fileName.mid(dot + 1) : QString();
    }
    case CreatedRole:
        return entry.created;
    case ModifiedRole:
        return entry.modified;
    case SelectedRole:
        return m_selectedIds.contains(entry.id);
    case CompareStatusRole:
        return static_cast<int>(entry.status);
    case GhostRole:
        return entry.isGhost;
    default:
        return {};
    }
}

/**
 * @brief Returns the role names exposed to QML.
 * @return Mapping from role ids to role names.
 */
QHash<int, QByteArray> FolderCompareSideModel::roleNames() const
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
 * @brief Requests a refresh of the compare model.
 */
void FolderCompareSideModel::refresh()
{
    if (m_compareModel) {
        m_compareModel->requestRefresh();
    }
}

/**
 * @brief Activates the entry at the given row.
 * @param row Row index to activate.
 */
void FolderCompareSideModel::activate(int row)
{
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

/**
 * @brief Selects or toggles the entry at the given row.
 * @param row Row index to select.
 * @param multi True to toggle selection, false to replace it.
 */
void FolderCompareSideModel::select(int row, bool multi)
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return;
    }
    const QString id = entry->id;
    if (!multi) {
        if (m_selectedIds.size() == 1 && m_selectedIds.first() == id) {
            return;
        }
        m_selectedIds = {id};
        notifySelectionChanged();
        return;
    }
    if (m_selectedIds.contains(id)) {
        m_selectedIds.removeAll(id);
    } else {
        m_selectedIds.append(id);
    }
    notifySelectionChanged();
}

/**
 * @brief Checks whether the entry at the given row is selected.
 * @param row Row index to inspect.
 * @return True if the row is selected, false otherwise.
 */
bool FolderCompareSideModel::isSelected(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return false;
    }
    return m_selectedIds.contains(entry->id);
}

/**
 * @brief Checks whether the entry at the given row is a folder.
 * @param row Row index to inspect.
 * @return True if the row is a folder, false otherwise.
 */
bool FolderCompareSideModel::isDir(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return false;
    }
    return entry->isDir;
}

/**
 * @brief Returns the absolute path for a row.
 * @param row Row index to inspect.
 * @return Absolute path for the row.
 */
QString FolderCompareSideModel::pathForRow(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return {};
    }
    return entry->filePath;
}

/**
 * @brief Formats the last modified time for a row.
 * @param row Row index to inspect.
 * @return Localized short format timestamp string.
 */
QString FolderCompareSideModel::modifiedForRow(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return {};
    }
    return QLocale().toString(entry->modified, QLocale::ShortFormat);
}

/**
 * @brief Clears the current selection.
 */
void FolderCompareSideModel::clearSelection()
{
    if (m_selectedIds.isEmpty()) {
        return;
    }
    m_selectedIds.clear();
    notifySelectionChanged();
}

/**
 * @brief Navigates to the parent folder of the current root path.
 */
void FolderCompareSideModel::goUp()
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
 * @brief Sets the selection from a list of row indices.
 * @param rows List of row indices to select.
 * @param additive True to add to current selection, false to replace it.
 */
void FolderCompareSideModel::setSelection(const QVariantList &rows, bool additive)
{
    QStringList next;
    if (additive) {
        next = m_selectedIds;
    }
    for (const QVariant &value : rows) {
        const int row = value.toInt();
        const CompareEntry *entry = entryForRow(row);
        if (!entry) {
            continue;
        }
        if (!next.contains(entry->id)) {
            next.append(entry->id);
        }
    }
    if (next == m_selectedIds) {
        return;
    }
    m_selectedIds = next;
    notifySelectionChanged();
}

/**
 * @brief Sets the selection for a continuous row range.
 * @param start Start row index.
 * @param end End row index.
 * @param additive True to add to current selection, false to replace it.
 */
void FolderCompareSideModel::setSelectionRange(int start, int end, bool additive)
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
bool FolderCompareSideModel::allSelected() const
{
    if (m_entries.isEmpty()) {
        return false;
    }
    return m_selectedIds.size() == m_entries.size();
}

/**
 * @brief Returns the row indices of selected entries.
 * @return List of selected row indices.
 */
QVariantList FolderCompareSideModel::selectedRows() const
{
    QVariantList rows;
    if (m_entries.isEmpty() || m_selectedIds.isEmpty()) {
        return rows;
    }
    rows.reserve(m_selectedIds.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_selectedIds.contains(m_entries.at(i).id)) {
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
bool FolderCompareSideModel::isImage(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return false;
    }
    return entry->isImage && !entry->isDir;
}

/**
 * @brief Checks whether the entry at the given row is a ghost item.
 * @param row Row index to inspect.
 * @return True if the row is a ghost item, false otherwise.
 */
bool FolderCompareSideModel::isGhost(int row) const
{
    const CompareEntry *entry = entryForRow(row);
    if (!entry) {
        return false;
    }
    return entry->isGhost;
}

/**
 * @brief Returns the compare status for the current selection.
 * @return Compare status value.
 */
int FolderCompareSideModel::selectedCompareStatus() const
{
    if (m_selectedIds.isEmpty()) {
        return StatusNone;
    }
    const QString id = m_selectedIds.first();
    auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const CompareEntry &entry) {
        return entry.id == id;
    });
    if (it == m_entries.end()) {
        return StatusNone;
    }
    return it->status;
}

/**
 * @brief Returns whether the current selection is a ghost item.
 * @return True if the selection is ghost, false otherwise.
 */
bool FolderCompareSideModel::selectedIsGhost() const
{
    if (m_selectedIds.isEmpty()) {
        return false;
    }
    const QString id = m_selectedIds.first();
    auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const CompareEntry &entry) {
        return entry.id == id;
    });
    if (it == m_entries.end()) {
        return false;
    }
    return it->isGhost;
}

/**
 * @brief Computes counts of selected folders and files.
 * @return Map with "dirs" and "files" counts.
 */
QVariantMap FolderCompareSideModel::selectionStats() const
{
    QVariantMap result;
    int dirs = 0;
    int files = 0;
    for (const QString &id : m_selectedIds) {
        auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const CompareEntry &entry) {
            return entry.id == id;
        });
        if (it == m_entries.end()) {
            continue;
        }
        const CompareEntry &entry = *it;
        if (entry.isGhost || entry.filePath.isEmpty()) {
            continue;
        }
        countPathRecursive(entry.filePath, dirs, files);
    }
    result.insert("dirs", dirs);
    result.insert("files", files);
    return result;
}

/**
 * @brief Copies selected items to a target folder synchronously.
 * @param targetDir Target folder path.
 * @return Result map including ok, copied, failed, and error fields.
 */
QVariantMap FolderCompareSideModel::copySelectedTo(const QString &targetDir)
{
    QVariantMap result;
    result.insert("ok", false);
    if (targetDir.isEmpty() || !QDir(targetDir).exists()) {
        result.insert("error", tr("Target folder not found"));
        return result;
    }

    const int total = m_selectedIds.size();
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

    for (const QString &id : m_selectedIds) {
        auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const CompareEntry &entry) {
            return entry.id == id;
        });
        if (it == m_entries.end()) {
            finishItem();
            continue;
        }
        const CompareEntry &entry = *it;
        if (entry.isGhost) {
            failed += 1;
            if (firstError.isEmpty()) {
                firstError = tr("Cannot copy ghost items");
            }
            finishItem();
            continue;
        }
        if (entry.filePath.isEmpty()) {
            failed += 1;
            if (firstError.isEmpty()) {
                firstError = tr("Source not found");
            }
            finishItem();
            continue;
        }
        const QFileInfo info(entry.filePath);
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
        if (entry.isDir) {
            QString error;
            if (!copyDirectoryRecursive(entry.filePath, targetPath, &error)) {
                failed += 1;
                if (firstError.isEmpty()) {
                    firstError = error.isEmpty() ? tr("Copy failed") : error;
                }
                finishItem();
                continue;
            }
            copied += 1;
        } else {
            if (!QFile::copy(entry.filePath, targetPath)) {
                failed += 1;
                if (firstError.isEmpty()) {
                    firstError = tr("Copy failed");
                }
                finishItem();
                continue;
            }
            const QFileInfo sourceInfo(entry.filePath);
            applyFileTimes(sourceInfo, targetPath);
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

/**
 * @brief Starts an asynchronous copy of selected items to a target folder.
 * @param targetDir Target folder path.
 */
void FolderCompareSideModel::startCopySelectedTo(const QString &targetDir)
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
    if (m_selectedIds.isEmpty()) {
        QVariantMap result;
        result.insert("ok", false);
        result.insert("error", tr("No items selected"));
        emit copyFinished(result);
        return;
    }

    QList<CopyItem> items;
    m_copyExtraFailed = 0;
    m_copyExtraError.clear();
    const QDir dir(targetDir);

    for (const QString &id : m_selectedIds) {
        auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const CompareEntry &entry) {
            return entry.id == id;
        });
        if (it == m_entries.end()) {
            m_copyExtraFailed += 1;
            if (m_copyExtraError.isEmpty()) {
                m_copyExtraError = tr("Source not found");
            }
            continue;
        }
        const CompareEntry &entry = *it;
        if (entry.isGhost) {
            m_copyExtraFailed += 1;
            if (m_copyExtraError.isEmpty()) {
                m_copyExtraError = tr("Cannot copy ghost items");
            }
            continue;
        }
        if (entry.filePath.isEmpty()) {
            m_copyExtraFailed += 1;
            if (m_copyExtraError.isEmpty()) {
                m_copyExtraError = tr("Source not found");
            }
            continue;
        }
        const QFileInfo info(entry.filePath);
        if (!info.exists()) {
            m_copyExtraFailed += 1;
            if (m_copyExtraError.isEmpty()) {
                m_copyExtraError = tr("Source not found");
            }
            continue;
        }
        CopyItem item;
        item.sourcePath = entry.filePath;
        item.targetPath = dir.filePath(info.fileName());
        item.isDir = entry.isDir;
        items.append(item);
    }

    if (items.isEmpty()) {
        QVariantMap result;
        result.insert("ok", false);
        result.insert("failed", m_copyExtraFailed);
        if (!m_copyExtraError.isEmpty()) {
            result.insert("error", m_copyExtraError);
        }
        emit copyFinished(result);
        m_copyExtraFailed = 0;
        m_copyExtraError.clear();
        return;
    }

    auto *thread = new QThread(this);
    auto *worker = new CopyWorker(items);
    worker->moveToThread(thread);

    m_copyThread = thread;
    m_copyWorker = worker;
    setCopyInProgress(true);
    updateCopyProgress(0, 0);

    connect(thread, &QThread::started, worker, &CopyWorker::start);
    connect(worker, &CopyWorker::progress, this, &FolderCompareSideModel::updateCopyProgress);
    connect(worker, &CopyWorker::finished, this, [this, thread, worker](QVariantMap result) {
        int failed = result.value("failed").toInt();
        failed += m_copyExtraFailed;
        result.insert("failed", failed);
        if (!m_copyExtraError.isEmpty() && !result.contains("error")) {
            result.insert("error", m_copyExtraError);
        }
        if (failed > 0 || result.value("cancelled").toBool()) {
            result.insert("ok", false);
        }
        setCopyInProgress(false);
        updateCopyProgress(0, 0);
        emit copyFinished(result);
        m_copyExtraFailed = 0;
        m_copyExtraError.clear();
        m_copyWorker = nullptr;
        m_copyThread = nullptr;
        thread->quit();
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

/**
 * @brief Requests cancellation of the current copy operation.
 */
void FolderCompareSideModel::cancelCopy()
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
QVariantMap FolderCompareSideModel::moveSelectedToTrash()
{
    QVariantMap result;
    if (m_selectedIds.isEmpty()) {
        result.insert("ok", false);
        result.insert("error", tr("Nothing to delete"));
        return result;
    }
    startMoveSelectedToTrash();
    result.insert("ok", true);
    result.insert("pending", FolderCompareTrashConstants::pendingTrue);
    return result;
}

/**
 * @brief Starts an asynchronous trash operation for selected items.
 */
void FolderCompareSideModel::startMoveSelectedToTrash()
{
    if (m_trashInProgress) {
        return;
    }

    QStringList paths;
    paths.reserve(m_selectedIds.size());
    int preFailed = FolderCompareTrashConstants::emptyCount;
    QString preError;
    for (const QString &id : m_selectedIds) {
        auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const CompareEntry &entry) {
            return entry.id == id;
        });
        if (it == m_entries.end()) {
            continue;
        }
        const CompareEntry &entry = *it;
        if (entry.isGhost) {
            preFailed += 1;
            if (preError.isEmpty()) {
                preError = tr("Cannot trash ghost items");
            }
            continue;
        }
        if (entry.filePath.isEmpty() || !QFileInfo::exists(entry.filePath)) {
            preFailed += 1;
            if (preError.isEmpty()) {
                preError = tr("Source not found");
            }
            continue;
        }
        paths.append(entry.filePath);
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

    auto *thread = new QThread(this);
    auto *worker = new TrashWorker(paths);
    worker->moveToThread(thread);

    m_trashThread = thread;
    m_trashWorker = worker;
    setTrashInProgress(true);
    updateTrashProgress(FolderCompareTrashConstants::emptyCount, paths.size());

    connect(thread, &QThread::started, worker, &TrashWorker::start);
    connect(worker, &TrashWorker::progress, this, &FolderCompareSideModel::updateTrashProgress);
    connect(worker, &TrashWorker::finished, this, [this, thread, preFailed, preError](QVariantMap result) {
        int failed = result.value("failed").toInt();
        failed += preFailed;
        result.insert("failed", failed);
        if (!preError.isEmpty() && !result.contains("error")) {
            result.insert("error", preError);
        }
        if (failed > FolderCompareTrashConstants::emptyCount || result.value("cancelled").toBool()) {
            result.insert("ok", false);
        }

        setTrashInProgress(false);
        updateTrashProgress(FolderCompareTrashConstants::emptyCount, FolderCompareTrashConstants::emptyCount);
        emit trashFinished(result);

        const int moved = result.value("moved").toInt();
        const QStringList touchedPaths = result.value("touchedPaths").toStringList();
        if (moved > FolderCompareTrashConstants::emptyCount && m_compareModel) {
            if (m_side == FolderCompareModel::Left) {
                m_compareModel->refreshFiles(touchedPaths, {});
            } else {
                m_compareModel->refreshFiles({}, touchedPaths);
            }
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
void FolderCompareSideModel::cancelTrash()
{
    if (!m_trashWorker) {
        return;
    }
    QMetaObject::invokeMethod(m_trashWorker, "cancel", Qt::QueuedConnection);
}

/**
 * @brief Renames a file or folder within the current compare side.
 * @param path Full path to rename.
 * @param newName New file or folder name (without path).
 * @return Result map including ok, newPath, and error fields.
 */
QVariantMap FolderCompareSideModel::renamePath(const QString &path, const QString &newName)
{
    QVariantMap result;
    result.insert("ok", false);

    if (path.isEmpty()) {
        result.insert("error", tr("Source not found"));
        return result;
    }

    auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const CompareEntry &entry) {
        return entry.filePath == path;
    });
    if (it != m_entries.end() && it->isGhost) {
        result.insert("error", tr("Cannot rename ghost items"));
        return result;
    }

    QString targetPath;
    QString error;
    if (!PlatformUtils::renamePath(path, newName, &targetPath, &error)) {
        result.insert("error", error.isEmpty() ? tr("Rename failed") : error);
        return result;
    }

    const int selectedIndex = m_selectedIds.indexOf(path);
    if (selectedIndex >= 0) {
        m_selectedIds[selectedIndex] = targetPath;
    }
    notifySelectionChanged();

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

/**
 * @brief Updates the loading state and emits change notification.
 * @param loading True when loading is active, false otherwise.
 */
void FolderCompareSideModel::setLoading(bool loading)
{
    if (loading == m_loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

/**
 * @brief Updates the copy-in-progress state and emits change notification.
 * @param inProgress True when copy is active, false otherwise.
 */
void FolderCompareSideModel::setCopyInProgress(bool inProgress)
{
    if (m_copyInProgress == inProgress) {
        return;
    }
    m_copyInProgress = inProgress;
    emit copyInProgressChanged();
}

/**
 * @brief Updates copy counters and emits progress when it changes.
 * @param completed Number of completed entries.
 * @param total Total number of entries to copy.
 */
void FolderCompareSideModel::updateCopyProgress(int completed, int total)
{
    m_copyCompleted = completed;
    m_copyTotal = total;
    const qreal nextProgress = total > 0
        ? static_cast<qreal>(completed) / static_cast<qreal>(total)
        : FolderCompareTrashConstants::zeroProgress;
    if (!qFuzzyCompare(m_copyProgress, nextProgress)) {
        m_copyProgress = nextProgress;
        emit copyProgressChanged();
    }
}

/**
 * @brief Updates the trash-in-progress state and emits change notification.
 * @param inProgress True when trash is active, false otherwise.
 */
void FolderCompareSideModel::setTrashInProgress(bool inProgress)
{
    if (m_trashInProgress == inProgress) {
        return;
    }
    m_trashInProgress = inProgress;
    emit trashInProgressChanged();
}

/**
 * @brief Updates trash counters and emits progress when it changes.
 * @param completed Number of completed entries.
 * @param total Total number of entries to trash.
 */
void FolderCompareSideModel::updateTrashProgress(int completed, int total)
{
    m_trashCompleted = completed;
    m_trashTotal = total;
    const qreal nextProgress = total > FolderCompareTrashConstants::emptyCount
        ? static_cast<qreal>(completed) / static_cast<qreal>(total)
        : FolderCompareTrashConstants::zeroProgress;
    if (!qFuzzyCompare(m_trashProgress, nextProgress)) {
        m_trashProgress = nextProgress;
        emit trashProgressChanged();
    }
}

/**
 * @brief Sets the base entries and rebuilds the filtered model.
 * @param entries Base compare entries to set.
 */
void FolderCompareSideModel::setBaseEntries(const QVector<CompareEntry> &entries)
{
    m_baseEntries = entries;
    QVector<CompareEntry> filtered = m_baseEntries;
    applyFilterAndSort(filtered);
    applyEntriesIncremental(filtered);
}

/**
 * @brief Updates base entries for affected names and paths only.
 * @param entries Updated compare entries.
 * @param affectedNames Set of affected names.
 * @param affectedPaths Set of affected paths.
 */
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
    QVector<CompareEntry> filtered = m_baseEntries;
    applyFilterAndSort(filtered);
    applyEntriesIncremental(filtered);
}

/**
 * @brief Rebuilds entries using current filters and sorting.
 */
void FolderCompareSideModel::rebuildEntries()
{
    QVector<CompareEntry> filtered = m_baseEntries;
    applyFilterAndSort(filtered);

    applyEntriesIncremental(filtered);
}

/**
 * @brief Applies an incremental update of entries to the model.
 * @param entries New entries to apply.
 */
void FolderCompareSideModel::applyEntriesIncremental(const QVector<CompareEntry> &entries)
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
        const QString &nextId = entries.at(i).id;
        if (i < m_entries.size() && m_entries.at(i).id == nextId) {
            if (!compareEntryEquivalent(m_entries.at(i), entries.at(i))) {
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
            if (m_entries.at(j).id == nextId) {
                existing = j;
                break;
            }
        }

        if (existing >= 0) {
            beginMoveRows(QModelIndex(), existing, existing, QModelIndex(), i);
            const CompareEntry moved = m_entries.takeAt(existing);
            m_entries.insert(i, moved);
            endMoveRows();
            if (!compareEntryEquivalent(m_entries.at(i), entries.at(i))) {
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

    QStringList nextSelection;
    nextSelection.reserve(m_selectedIds.size());
    for (const QString &id : m_selectedIds) {
        auto it = std::find_if(entries.begin(), entries.end(), [&](const CompareEntry &entry) {
            return entry.id == id;
        });
        if (it != entries.end()) {
            nextSelection.append(id);
        }
    }
    if (nextSelection != m_selectedIds) {
        m_selectedIds = nextSelection;
    }
    notifySelectionChanged();
}

/**
 * @brief Filters and sorts entries based on current settings.
 * @param entries Entries to filter and sort in place.
 */
void FolderCompareSideModel::applyFilterAndSort(QVector<CompareEntry> &entries) const
{
    if (m_hideIdentical) {
        entries.erase(std::remove_if(entries.begin(), entries.end(), [](const CompareEntry &entry) {
            return entry.status == StatusIdentical;
        }), entries.end());
    }

    const QString trimmed = m_nameFilter.trimmed();
    if (!trimmed.isEmpty()) {
        const QString needle = trimmed.toLower();
        entries.erase(std::remove_if(entries.begin(), entries.end(), [needle](const CompareEntry &entry) {
            return !entry.fileName.toLower().contains(needle);
        }), entries.end());
    }

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);

    auto compare = [&](const CompareEntry &left, const CompareEntry &right) {
        if (m_showDirsFirst && left.isDir != right.isDir) {
            return left.isDir;
        }

        bool result = false;
        switch (m_sortKey) {
        case Extension: {
            const QString leftSuffix = QFileInfo(left.fileName).suffix();
            const QString rightSuffix = QFileInfo(right.fileName).suffix();
            result = collator.compare(leftSuffix, rightSuffix) < 0;
            break;
        }
        case Created:
            result = left.created < right.created;
            break;
        case Modified:
            result = left.modified < right.modified;
            break;
        case Name:
        default:
            result = collator.compare(left.fileName, right.fileName) < 0;
            break;
        }

        return m_sortOrder == Qt::AscendingOrder ? result : !result;
    };

    std::sort(entries.begin(), entries.end(), compare);
}

/**
 * @brief Emits selection change signals and updates selection-dependent roles.
 */
void FolderCompareSideModel::notifySelectionChanged()
{
    rebuildSelectedPaths();
    emit selectedPathsChanged();

    const CompareEntry *entry = nullptr;
    if (m_selectedIds.size() == 1) {
        const QString &id = m_selectedIds.first();
        for (const CompareEntry &item : m_entries) {
            if (item.id == id) {
                entry = &item;
                break;
            }
        }
    }
    const bool nextIsImage = entry && entry->isImage && !entry->isGhost;
    if (nextIsImage != m_selectedIsImage) {
        m_selectedIsImage = nextIsImage;
        emit selectedIsImageChanged();
    }

    if (m_entries.isEmpty()) {
        return;
    }
    const QModelIndex first = index(0, 0);
    const QModelIndex last = index(m_entries.size() - 1, 0);
    emit dataChanged(first, last, {SelectedRole});
}

/**
 * @brief Rebuilds selected paths from selected entry identifiers.
 */
void FolderCompareSideModel::rebuildSelectedPaths()
{
    QStringList nextPaths;
    nextPaths.reserve(m_selectedIds.size());
    for (const QString &id : m_selectedIds) {
        auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const CompareEntry &entry) {
            return entry.id == id;
        });
        if (it == m_entries.end()) {
            continue;
        }
        if (!it->filePath.isEmpty() && !it->isGhost) {
            nextPaths.append(it->filePath);
        }
    }
    m_selectedPaths = nextPaths;
}

/**
 * @brief Returns the compare entry for a given row.
 * @param row Row index to inspect.
 * @return Pointer to the entry, or null when out of range.
 */
const FolderCompareSideModel::CompareEntry *FolderCompareSideModel::entryForRow(int row) const
{
    if (row < 0 || row >= m_entries.size()) {
        return nullptr;
    }
    return &m_entries.at(row);
}

/**
 * @brief Returns a mutable compare entry for a given row.
 * @param row Row index to inspect.
 * @return Pointer to the entry, or null when out of range.
 */
FolderCompareSideModel::CompareEntry *FolderCompareSideModel::entryForRow(int row)
{
    if (row < 0 || row >= m_entries.size()) {
        return nullptr;
    }
    return &m_entries[row];
}
