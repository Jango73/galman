#pragma once

#include <QString>

class QFileInfo;

namespace FileOperationUtils {

void applyFileTimes(const QFileInfo &sourceInfo, const QString &targetPath);
bool copyFolderRecursive(const QString &sourcePath, const QString &targetPath, const char *context, QString *error);

} // namespace FileOperationUtils
