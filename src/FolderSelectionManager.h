#pragma once

#include <QStringList>
#include <QVariantList>

#include <functional>

class FolderSelectionManager
{
public:
    FolderSelectionManager() = default;

    QStringList selectedKeys() const;
    void setSelectedKeys(const QStringList &keys);

    bool selectSingle(const QString &key);
    bool toggleKey(const QString &key);
    bool clearSelection();
    bool allSelected(int rowCount) const;
    bool isSelected(const QString &key) const;
    QVariantList selectedRows(int rowCount, const std::function<QString(int)> &keyForRow) const;

    template <typename KeyForRow>
    bool setFromRowsGeneric(const QVariantList &rows, bool additive, int rowCount, KeyForRow keyForRow)
    {
        QStringList next;
        if (additive) {
            next = m_selectedKeys;
        }
        for (const QVariant &value : rows) {
            const int row = value.toInt();
            if (row < 0 || row >= rowCount) {
                continue;
            }
            const QString key = keyForRow(row);
            if (key.isEmpty()) {
                continue;
            }
            if (!next.contains(key)) {
                next.append(key);
            }
        }
        if (next == m_selectedKeys) {
            return false;
        }
        m_selectedKeys = next;
        return true;
    }

    template <typename KeyForRow>
    QVariantList selectedRowsGeneric(int rowCount, KeyForRow keyForRow) const
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

private:
    QStringList m_selectedKeys;
};
