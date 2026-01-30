#pragma once

#include <QStringList>
#include <QtGlobal>
#include <functional>

class QObject;

namespace SelectionStatisticsUtils {

struct SelectionStatisticsResult {
    int folderCount = 0;
    int fileCount = 0;
    qint64 totalBytes = 0;
};

SelectionStatisticsResult computeStatistics(const QStringList &paths);
int countNameConflicts(const QStringList &paths, const QString &targetFolder);
void updateSelectionTotalsAsync(QObject *parent,
                                const QStringList &paths,
                                int *generation,
                                const std::function<void(int, qint64)> &applyTotals);

} // namespace SelectionStatisticsUtils
