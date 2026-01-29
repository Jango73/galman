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

#include "SelectionStatisticsUtils.h"

#include <QDir>
#include <QFileInfo>

namespace {

void accumulatePathStatistics(const QString &path, SelectionStatisticsUtils::SelectionStatisticsResult &result)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        return;
    }
    if (info.isDir()) {
        result.folderCount += 1;
        const QDir dir(path);
        const QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::NoSort);
        for (const QFileInfo &entry : entries) {
            accumulatePathStatistics(entry.absoluteFilePath(), result);
        }
    } else {
        result.fileCount += 1;
        result.totalBytes += info.size();
    }
}

} // namespace

namespace SelectionStatisticsUtils {

SelectionStatisticsResult computeStatistics(const QStringList &paths)
{
    SelectionStatisticsResult result;
    for (const QString &path : paths) {
        accumulatePathStatistics(path, result);
    }
    return result;
}

int countNameConflicts(const QStringList &paths, const QString &targetFolder)
{
    if (targetFolder.isEmpty()) {
        return 0;
    }
    const QDir dir(targetFolder);
    if (!dir.exists()) {
        return 0;
    }
    int conflicts = 0;
    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (!info.exists()) {
            continue;
        }
        const QString targetPath = dir.filePath(info.fileName());
        if (QFileInfo::exists(targetPath)) {
            conflicts += 1;
        }
    }
    return conflicts;
}

} // namespace SelectionStatisticsUtils
