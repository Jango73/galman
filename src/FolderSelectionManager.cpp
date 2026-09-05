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

#include "FolderSelectionManager.h"

QStringList FolderSelectionManager::selectedKeys() const
{
    return m_selectedKeys;
}

void FolderSelectionManager::setSelectedKeys(const QStringList &keys)
{
    m_selectedKeys = keys;
}

bool FolderSelectionManager::selectSingle(const QString &key)
{
    if (m_selectedKeys.size() == 1 && m_selectedKeys.first() == key) {
        return false;
    }
    m_selectedKeys = {key};
    return true;
}

bool FolderSelectionManager::toggleKey(const QString &key)
{
    if (m_selectedKeys.contains(key)) {
        m_selectedKeys.removeAll(key);
    } else {
        m_selectedKeys.append(key);
    }
    return true;
}

bool FolderSelectionManager::clearSelection()
{
    if (m_selectedKeys.isEmpty()) {
        return false;
    }
    m_selectedKeys.clear();
    return true;
}

bool FolderSelectionManager::allSelected(int rowCount) const
{
    if (rowCount <= 0) {
        return false;
    }
    return m_selectedKeys.size() == rowCount;
}

bool FolderSelectionManager::isSelected(const QString &key) const
{
    return m_selectedKeys.contains(key);
}

QVariantList FolderSelectionManager::selectedRows(int rowCount, const std::function<QString(int)> &keyForRow) const
{
    QVariantList rows;
    if (rowCount <= 0 || m_selectedKeys.isEmpty()) {
        return rows;
    }
    rows.reserve(m_selectedKeys.size());
    for (int row = 0; row < rowCount; ++row) {
        if (m_selectedKeys.contains(keyForRow(row))) {
            rows.append(row);
        }
    }
    return rows;
}
