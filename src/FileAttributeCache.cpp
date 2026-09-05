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

#include "FileAttributeCache.h"

FileAttributeCache::FileAttributeCache(QObject *parent)
    : QObject(parent)
{
}

const QHash<QString, QSize> &FileAttributeCache::imageSizes() const
{
    return m_imageSizes;
}

const QHash<QString, quint64> &FileAttributeCache::signatureHashes() const
{
    return m_signatureHashes;
}

const QHash<QString, QString> &FileAttributeCache::videoThumbnails() const
{
    return m_videoThumbnails;
}

void FileAttributeCache::setImageSize(const QString &path, const QSize &size)
{
    m_imageSizes.insert(path, size);
}

void FileAttributeCache::setSignatureHash(const QString &path, quint64 hash)
{
    m_signatureHashes.insert(path, hash);
}

void FileAttributeCache::setVideoThumbnail(const QString &path, const QString &thumbnail)
{
    m_videoThumbnails.insert(path, thumbnail);
}

bool FileAttributeCache::hasImageSize(const QString &path) const
{
    return m_imageSizes.contains(path);
}

bool FileAttributeCache::hasSignatureHash(const QString &path) const
{
    return m_signatureHashes.contains(path);
}

bool FileAttributeCache::signatureHashForPath(const QString &path, quint64 *value) const
{
    auto found = m_signatureHashes.constFind(path);
    if (found == m_signatureHashes.constEnd()) {
        return false;
    }
    if (value) {
        *value = found.value();
    }
    return true;
}

void FileAttributeCache::pruneCaches(const QSet<QString> &currentPaths)
{
    for (auto iterator = m_imageSizes.begin(); iterator != m_imageSizes.end();) {
        if (!currentPaths.contains(iterator.key())) {
            iterator = m_imageSizes.erase(iterator);
        } else {
            ++iterator;
        }
    }
    for (auto iterator = m_imageSizeAttempted.begin(); iterator != m_imageSizeAttempted.end();) {
        if (!currentPaths.contains(*iterator)) {
            iterator = m_imageSizeAttempted.erase(iterator);
        } else {
            ++iterator;
        }
    }
    for (auto iterator = m_signatureHashes.begin(); iterator != m_signatureHashes.end();) {
        if (!currentPaths.contains(iterator.key())) {
            iterator = m_signatureHashes.erase(iterator);
        } else {
            ++iterator;
        }
    }
    for (auto iterator = m_signatureAttempted.begin(); iterator != m_signatureAttempted.end();) {
        if (!currentPaths.contains(*iterator)) {
            iterator = m_signatureAttempted.erase(iterator);
        } else {
            ++iterator;
        }
    }
    for (auto iterator = m_videoThumbnails.begin(); iterator != m_videoThumbnails.end();) {
        if (!currentPaths.contains(iterator.key())) {
            iterator = m_videoThumbnails.erase(iterator);
        } else {
            ++iterator;
        }
    }
    for (auto iterator = m_videoThumbnailAttempted.begin(); iterator != m_videoThumbnailAttempted.end();) {
        if (!currentPaths.contains(*iterator)) {
            iterator = m_videoThumbnailAttempted.erase(iterator);
        } else {
            ++iterator;
        }
    }
}

bool FileAttributeCache::imageSizeAttempted(const QString &path) const
{
    return m_imageSizeAttempted.contains(path);
}

void FileAttributeCache::markImageSizeAttempted(const QString &path)
{
    m_imageSizeAttempted.insert(path);
}

bool FileAttributeCache::signatureAttempted(const QString &path) const
{
    return m_signatureAttempted.contains(path);
}

void FileAttributeCache::markSignatureAttempted(const QString &path)
{
    m_signatureAttempted.insert(path);
}

bool FileAttributeCache::videoThumbnailAttempted(const QString &path) const
{
    return m_videoThumbnailAttempted.contains(path);
}

void FileAttributeCache::markVideoThumbnailAttempted(const QString &path)
{
    m_videoThumbnailAttempted.insert(path);
}

void FileAttributeCache::setImageSizeLoading(bool loading)
{
    m_imageSizeLoading = loading;
}

bool FileAttributeCache::imageSizeLoading() const
{
    return m_imageSizeLoading;
}

void FileAttributeCache::setSignatureLoading(bool loading)
{
    m_signatureLoading = loading;
}

bool FileAttributeCache::signatureLoading() const
{
    return m_signatureLoading;
}

void FileAttributeCache::setVideoThumbnailLoading(bool loading)
{
    m_videoThumbnailLoading = loading;
}

bool FileAttributeCache::videoThumbnailLoading() const
{
    return m_videoThumbnailLoading;
}

int FileAttributeCache::imageSizeGeneration() const
{
    return m_imageSizeGeneration;
}

void FileAttributeCache::setImageSizeGeneration(int generation)
{
    m_imageSizeGeneration = generation;
}

int FileAttributeCache::signatureGeneration() const
{
    return m_signatureGeneration;
}

void FileAttributeCache::setSignatureGeneration(int generation)
{
    m_signatureGeneration = generation;
}

int FileAttributeCache::videoThumbnailGeneration() const
{
    return m_videoThumbnailGeneration;
}

void FileAttributeCache::setVideoThumbnailGeneration(int generation)
{
    m_videoThumbnailGeneration = generation;
}
