#pragma once

#include <QString>

namespace PlatformUtils {

QString normalizePath(const QString &path);
QString comfyBaseFolder();
QString comfyModelsFolder();
QString comfyDefaultOutputDir();
bool moveToTrashOrDelete(const QString &path, QString *error);
bool deletePermanently(const QString &path, QString *error);
bool renamePath(const QString &path, const QString &newName, QString *newPath, QString *error);

} // namespace PlatformUtils
