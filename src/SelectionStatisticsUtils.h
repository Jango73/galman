#pragma once

#include <QStringList>
#include <QtGlobal>

namespace SelectionStatisticsUtils {

struct SelectionStatisticsResult {
    int folderCount = 0;
    int fileCount = 0;
    qint64 totalBytes = 0;
};

SelectionStatisticsResult computeStatistics(const QStringList &paths);
int countNameConflicts(const QStringList &paths, const QString &targetFolder);

} // namespace SelectionStatisticsUtils
