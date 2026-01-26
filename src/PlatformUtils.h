#pragma once

#include <QString>

namespace PlatformUtils {

QString normalizePath(const QString &path);
QString comfyDefaultOutputDir();
bool moveToTrashOrDelete(const QString &path, QString *error);

} // namespace PlatformUtils
