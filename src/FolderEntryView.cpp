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

#include "FolderEntryView.h"

namespace FolderEntryView {

EntryView fromFileInfo(const QFileInfo &info, bool isImage, bool isVideo)
{
    EntryView view;
    view.fileName = info.fileName();
    view.filePath = info.absoluteFilePath();
    view.isFolder = info.isDir();
    view.isGhost = false;
    view.createdTime = info.birthTime().isValid() ? info.birthTime() : info.lastModified();
    view.modifiedTime = info.lastModified();
    view.isImageFlag = isImage;
    view.isVideoFlag = isVideo;
    view.byteSize = info.isDir() ? -1 : info.size();
    return view;
}

QString suffixForFileName(const QString &fileName)
{
    const int dot = fileName.lastIndexOf(QLatin1Char('.'));
    if (dot < 0) {
        return QString();
    }
    return fileName.mid(dot + 1);
}

bool matchesJunkExtension(const QString &suffix, const QStringList &junkExtensions)
{
    for (const QString &extension : junkExtensions) {
        QString normalized = extension;
        if (normalized.startsWith(QLatin1Char('.'))) {
            normalized = normalized.mid(1);
        }
        if (suffix.compare(normalized, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

bool matchesNameFilter(const QString &fileName, const QString &needle)
{
    if (needle.isEmpty()) {
        return true;
    }
    return fileName.toLower().contains(needle);
}

} // namespace FolderEntryView
