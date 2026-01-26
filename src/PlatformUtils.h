#pragma once

#include <QString>

namespace PlatformUtils {

QString normalizePath(const QString &path);
QString comfyDefaultOutputDir();
bool moveToTrashOrDelete(const QString &path, QString *error);
bool renamePath(const QString &path, const QString &newName, QString *newPath, QString *error);

} // namespace PlatformUtils
