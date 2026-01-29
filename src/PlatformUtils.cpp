
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

namespace {

struct RemovalConstants {
    static constexpr bool allowTrash = true;
    static constexpr bool skipTrash = false;
};

bool removePathInternal(const QString &path, bool allowTrash, const QString &failureMessage, QString *error)
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
    if (allowTrash && QFile::moveToTrash(path)) {
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
        *error = failureMessage;
    }
    return ok;
}

} // namespace

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
    const QString failureMessage = QCoreApplication::translate("PlatformUtils", "Failed to move to trash");
    return removePathInternal(path, RemovalConstants::allowTrash, failureMessage, error);
}

bool deletePermanently(const QString &path, QString *error)
{
    const QString failureMessage = QCoreApplication::translate("PlatformUtils", "Failed to delete");
    return removePathInternal(path, RemovalConstants::skipTrash, failureMessage, error);
}

/**
 * @brief Renames a file or folder within its parent folder.
 * @param path Existing file or folder path.
 * @param newName New name without path separators.
 * @param newPath Optional output path for the renamed entry.
 * @param error Optional output error message.
 * @return True if rename succeeds, false otherwise.
 */
bool renamePath(const QString &path, const QString &newName, QString *newPath, QString *error)
{
    if (path.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Source not found");
        }
        return false;
    }
    const QString trimmedName = newName.trimmed();
    if (trimmedName.isEmpty()) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Name cannot be empty");
        }
        return false;
    }

    const QChar forwardSlash = QLatin1Char('/');
    const QChar backSlash = QLatin1Char('\\');
    if (trimmedName.contains(forwardSlash) || trimmedName.contains(backSlash)) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Name cannot contain path separators");
        }
        return false;
    }

    const QFileInfo info(path);
    if (!info.exists()) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Source not found");
        }
        return false;
    }

    QDir parentDir = info.dir();
    const QString targetPath = parentDir.filePath(trimmedName);
    if (QDir::cleanPath(targetPath) == QDir::cleanPath(path)) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Name unchanged");
        }
        return false;
    }
    if (QFileInfo::exists(targetPath)) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Target already exists");
        }
        return false;
    }

    if (!parentDir.rename(info.fileName(), trimmedName)) {
        if (error) {
            *error = QCoreApplication::translate("PlatformUtils", "Rename failed");
        }
        return false;
    }

    if (newPath) {
        *newPath = targetPath;
    }
    return true;
}

} // namespace PlatformUtils
