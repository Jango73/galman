#pragma once

#include <QString>

namespace RowMatchUtils {

template <typename NameForRow>
int rowForPrefix(const QString &prefix, int startRow, int rowCount, NameForRow nameForRow)
{
    const QString trimmed = prefix.trimmed();
    if (trimmed.isEmpty() || rowCount <= 0) {
        return -1;
    }
    int start = startRow;
    if (start < 0) {
        start = 0;
    } else if (start >= rowCount) {
        start = rowCount - 1;
    }
    for (int i = start; i < rowCount; ++i) {
        if (nameForRow(i).startsWith(trimmed, Qt::CaseInsensitive)) {
            return i;
        }
    }
    for (int i = 0; i < start; ++i) {
        if (nameForRow(i).startsWith(trimmed, Qt::CaseInsensitive)) {
            return i;
        }
    }
    return -1;
}

} // namespace RowMatchUtils
