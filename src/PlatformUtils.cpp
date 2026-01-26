
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

#include "PlatformUtils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

namespace PlatformUtils {

/**
 * @brief Normalizes a path for consistent comparisons across platforms.
 * @param path Input path to normalize.
 * @return Normalized absolute path using native separators.
 */
QString normalizePath(const QString &path)
{
    QString normalized = QDir::fromNativeSeparators(path);
    normalized = QDir(normalized).absolutePath();
#ifdef Q_OS_WIN
    normalized = normalized.toLower();
#endif
    return normalized;
}

/**
 * @brief Returns the default ComfyUI output folder for the current platform.
 * @return Default output folder path.
 */
QString comfyDefaultOutputDir()
{
#ifdef Q_OS_WIN
    const QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString base = documents.isEmpty() ? QDir::home().filePath("Documents") : documents;
    return QDir(base).filePath("ComfyUI/output");
#else
    return QDir::home().filePath("ComfyUI/output");
#endif
}

/**
 * @brief Moves a path to trash when supported, otherwise deletes it.
 * @param path File or folder path to remove.
 * @param error Optional output error message.
 * @return True if removal succeeds, false otherwise.
 */
bool moveToTrashOrDelete(const QString &path, QString *error)
{
    if (path.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Path is empty");
        }
        return false;
    }
    if (!QFileInfo::exists(path)) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Source not found");
        }
        return false;
    }
    if (QFile::moveToTrash(path)) {
        return true;
    }
    QFileInfo info(path);
    bool ok = false;
    if (info.isDir()) {
        QDir dir(path);
        ok = dir.removeRecursively();
    } else {
        ok = QFile::remove(path);
    }
    if (!ok && error) {
        *error = QCoreApplication::translate("PlatformUtils", "Failed to move to trash");
    }
    return ok;
}

} // namespace PlatformUtils
