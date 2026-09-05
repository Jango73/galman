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

#include "FolderFilterSettings.h"

#include "ApplicationSettings.h"

QString FolderFilterSettings::nameFilter() const
{
    return m_nameFilter;
}

void FolderFilterSettings::setNameFilter(const QString &filter)
{
    m_nameFilter = filter;
}

Qt::SortOrder FolderFilterSettings::sortOrder() const
{
    return m_sortOrder;
}

void FolderFilterSettings::setSortOrder(Qt::SortOrder order)
{
    m_sortOrder = order;
}

int FolderFilterSettings::sortKeyValue() const
{
    return m_sortKeyValue;
}

void FolderFilterSettings::setSortKeyValue(int key)
{
    m_sortKeyValue = key;
}

bool FolderFilterSettings::showFoldersFirst() const
{
    return m_showFoldersFirst;
}

void FolderFilterSettings::setShowFoldersFirst(bool enabled)
{
    m_showFoldersFirst = enabled;
}

bool FolderFilterSettings::hideJunkFiles() const
{
    return m_hideJunkFiles;
}

void FolderFilterSettings::setHideJunkFiles(bool enabled)
{
    m_hideJunkFiles = enabled;
}

qint64 FolderFilterSettings::minimumByteSize() const
{
    return m_minimumByteSize;
}

void FolderFilterSettings::setMinimumByteSize(qint64 value)
{
    m_minimumByteSize = normalizedByteSize(value);
}

qint64 FolderFilterSettings::maximumByteSize() const
{
    return m_maximumByteSize;
}

void FolderFilterSettings::setMaximumByteSize(qint64 value)
{
    m_maximumByteSize = normalizedByteSize(value);
}

int FolderFilterSettings::minimumImageWidth() const
{
    return m_minimumImageWidth;
}

void FolderFilterSettings::setMinimumImageWidth(int value)
{
    m_minimumImageWidth = normalizedDimension(value);
}

int FolderFilterSettings::maximumImageWidth() const
{
    return m_maximumImageWidth;
}

void FolderFilterSettings::setMaximumImageWidth(int value)
{
    m_maximumImageWidth = normalizedDimension(value);
}

int FolderFilterSettings::minimumImageHeight() const
{
    return m_minimumImageHeight;
}

void FolderFilterSettings::setMinimumImageHeight(int value)
{
    m_minimumImageHeight = normalizedDimension(value);
}

int FolderFilterSettings::maximumImageHeight() const
{
    return m_maximumImageHeight;
}

void FolderFilterSettings::setMaximumImageHeight(int value)
{
    m_maximumImageHeight = normalizedDimension(value);
}

bool FolderFilterSettings::byteSizeFiltersActive() const
{
    return m_minimumByteSize > unsetByteSize || m_maximumByteSize > unsetByteSize;
}

bool FolderFilterSettings::imageSizeFiltersActive() const
{
    return m_minimumImageWidth > unsetDimension || m_maximumImageWidth > unsetDimension
        || m_minimumImageHeight > unsetDimension || m_maximumImageHeight > unsetDimension;
}

QStringList FolderFilterSettings::junkExtensions()
{
    ApplicationSettings settings;
    return settings.value("junkFiles/extensions", ".jpg~,.png~,.blend1")
        .toString()
        .split(QLatin1Char(','), Qt::SkipEmptyParts);
}

QString FolderFilterSettings::junkExtensionsString()
{
    ApplicationSettings settings;
    return settings.value("junkFiles/extensions", ".jpg~,.png~,.blend1").toString();
}

void FolderFilterSettings::setJunkExtensionsList(const QString &extensions)
{
    ApplicationSettings settings;
    settings.setValue("junkFiles/extensions", extensions);
}

qint64 FolderFilterSettings::normalizedByteSize(qint64 value)
{
    return value < 0 ? unsetByteSize : value;
}

int FolderFilterSettings::normalizedDimension(int value)
{
    return value < 0 ? unsetDimension : value;
}
