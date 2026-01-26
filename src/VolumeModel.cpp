
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

#include "VolumeModel.h"

#include <QDir>
#include <QSet>
#include <QStorageInfo>

#include "PlatformUtils.h"

/**
 * @brief Constructs the volume model and loads initial entries.
 * @param parent Parent QObject for ownership.
 */
VolumeModel::VolumeModel(QObject *parent)
    : QAbstractListModel(parent)
{
    loadVolumes();
}

/**
 * @brief Returns the number of rows for the model.
 * @param parent Parent index (unused for list model).
 * @return Number of rows.
 */
int VolumeModel::rowCount(const QModelIndex &parent) const
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
QVariant VolumeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const VolumeEntry &entry = m_entries.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case LabelRole:
        return entry.label;
    case PathRole:
        return entry.path;
    default:
        return {};
    }
}

/**
 * @brief Returns the role names exposed to QML.
 * @return Mapping from role ids to role names.
 */
QHash<int, QByteArray> VolumeModel::roleNames() const
{
    return {
        {LabelRole, "label"},
        {PathRole, "path"},
    };
}

/**
 * @brief Reloads the list of mounted volumes.
 */
void VolumeModel::refresh()
{
    loadVolumes();
}

/**
 * @brief Returns the path for a given row index.
 * @param index Row index to resolve.
 * @return Volume path, or an empty string if out of range.
 */
QString VolumeModel::pathForIndex(int index) const
{
    if (index < 0 || index >= m_entries.size()) {
        return {};
    }
    return m_entries.at(index).path;
}

/**
 * @brief Finds the row index that matches a given path.
 * @param path Path to resolve.
 * @return Row index for the path, or -1 if not found.
 */
int VolumeModel::indexForPath(const QString &path) const
{
    if (path.isEmpty()) {
        return -1;
    }
    const QString normalized = PlatformUtils::normalizePath(path);
    for (int i = 0; i < m_entries.size(); ++i) {
        const QString entryPath = PlatformUtils::normalizePath(m_entries.at(i).path);
        if (entryPath == "/" || entryPath.isEmpty()) {
            if (normalized.startsWith("/")) {
                return i;
            }
            continue;
        }
        if (normalized == entryPath || normalized.startsWith(entryPath + "/")) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Scans mounted volumes and updates the model entries.
 */
void VolumeModel::loadVolumes()
{
    QSet<QString> seen;
    QVector<VolumeEntry> nextEntries;
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const QStorageInfo &info : volumes) {
        if (!info.isValid() || !info.isReady()) {
            continue;
        }
        QString rootPath = info.rootPath();
        if (rootPath.isEmpty()) {
            continue;
        }
        rootPath = QDir::fromNativeSeparators(rootPath);
        if (seen.contains(rootPath)) {
            continue;
        }
        seen.insert(rootPath);

        const QString displayName = info.displayName().trimmed();
        VolumeEntry entry;
        entry.path = rootPath;
        if (!displayName.isEmpty() && displayName != rootPath) {
            entry.label = QString("%1 (%2)").arg(displayName, rootPath);
        } else {
            entry.label = rootPath;
        }
        nextEntries.push_back(entry);
    }
    applyEntriesIncremental(nextEntries);
}

/**
 * @brief Applies an incremental update of volume entries.
 * @param entries New entries to apply.
 */
void VolumeModel::applyEntriesIncremental(const QVector<VolumeEntry> &entries)
{
    const QVector<int> dataRoles = {LabelRole, PathRole};
    auto entryEquivalent = [](const VolumeEntry &left, const VolumeEntry &right) {
        return left.path == right.path && left.label == right.label;
    };
    int i = 0;
    while (i < entries.size()) {
        const QString &nextPath = entries.at(i).path;
        if (i < m_entries.size() && m_entries.at(i).path == nextPath) {
            if (!entryEquivalent(m_entries.at(i), entries.at(i))) {
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
            if (m_entries.at(j).path == nextPath) {
                existing = j;
                break;
            }
        }

        if (existing >= 0) {
            beginMoveRows(QModelIndex(), existing, existing, QModelIndex(), i);
            const VolumeEntry moved = m_entries.takeAt(existing);
            m_entries.insert(i, moved);
            endMoveRows();
            if (!entryEquivalent(m_entries.at(i), entries.at(i))) {
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
