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

#include "FolderFilterSortUtils.h"

namespace FolderFilterSortUtils {

bool matchesByteSize(qint64 byteSize, bool isFolder, const FolderFilterSettings &settings)
{
    if (isFolder) {
        return true;
    }
    if (settings.minimumByteSize() > FolderFilterSettings::unsetByteSize && byteSize < settings.minimumByteSize()) {
        return false;
    }
    if (settings.maximumByteSize() > FolderFilterSettings::unsetByteSize && byteSize > settings.maximumByteSize()) {
        return false;
    }
    return true;
}

bool matchesImageSize(const QString &imageKey,
                      bool isImage,
                      bool isFolder,
                      bool isGhost,
                      const FolderFilterSettings &settings,
                      const QHash<QString, QSize> &imageSizes)
{
    if (!isImage || isFolder || isGhost || imageKey.isEmpty()) {
        return false;
    }
    if (!imageSizes.contains(imageKey)) {
        return false;
    }
    const QSize size = imageSizes.value(imageKey);
    if (!size.isValid()) {
        return false;
    }
    if (settings.minimumImageWidth() > FolderFilterSettings::unsetDimension && size.width() < settings.minimumImageWidth()) {
        return false;
    }
    if (settings.maximumImageWidth() > FolderFilterSettings::unsetDimension && size.width() > settings.maximumImageWidth()) {
        return false;
    }
    if (settings.minimumImageHeight() > FolderFilterSettings::unsetDimension
        && size.height() < settings.minimumImageHeight()) {
        return false;
    }
    if (settings.maximumImageHeight() > FolderFilterSettings::unsetDimension
        && size.height() > settings.maximumImageHeight()) {
        return false;
    }
    return true;
}

bool matchesNameAndJunk(const FolderEntryView::EntryView &view,
                        const QString &needle,
                        bool nameFilterActive,
                        bool junkFilterActive,
                        const QStringList &junkExtensions)
{
    if (junkFilterActive && !view.isFolder) {
        const QString suffix = FolderEntryView::suffixForFileName(view.fileName);
        if (!suffix.isEmpty() && FolderEntryView::matchesJunkExtension(suffix, junkExtensions)) {
            return false;
        }
    }
    if (nameFilterActive && !FolderEntryView::matchesNameFilter(view.fileName, needle)) {
        return false;
    }
    return true;
}

} // namespace FolderFilterSortUtils
