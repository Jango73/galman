
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

#include "FolderCompareModel.h"

#include <QCollator>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFutureWatcher>
#include <QImageReader>
#include <QIODevice>
#include <QLocale>
#include <QSet>
#include <QThread>
#include <QtConcurrent>
#include <algorithm>
#include <cmath>

namespace {

struct ImageSignature {
    bool valid = false;
    QImage image;
};

struct MatchEntry {
    int entryIndex = -1;
    QString name;
    qint64 modified = 0;
    bool isDir = false;
    ImageSignature signature;
    bool signatureAttempted = false;
    bool matched = false;
};

struct CompareResult {
    QVector<FolderCompareSideModel::CompareEntry> leftEntries;
    QVector<FolderCompareSideModel::CompareEntry> rightEntries;
};

struct RefreshResult {
    QHash<QString, FolderCompareModel::CachedEntry> leftCache;
    QHash<QString, FolderCompareModel::CachedEntry> rightCache;
    CompareResult result;
};


ImageSignature readSignature(const QString &path);
bool isPathInRoot(const QString &rootPath, const QString &path);
QHash<QString, FolderCompareModel::CachedEntry> scanCache(const QString &rootPath, bool enabled, bool computeSignatures);
void updateCacheForPaths(QHash<QString, FolderCompareModel::CachedEntry> &cache,
                         const QStringList &paths,
                         const QString &rootPath,
                         bool enabled,
                         bool computeSignatures);
CompareResult compareCaches(const QHash<QString, FolderCompareModel::CachedEntry> &leftCache,
                            const QHash<QString, FolderCompareModel::CachedEntry> &rightCache,
                            bool enabled);

/**
 * @brief Returns the set of supported image file extensions.
 * @return Set of lowercase image file extensions.
 */
const QSet<QString> &supportedImageFormats()
{
    static const QSet<QString> formats = []() {
        QSet<QString> set;
        const auto supported = QImageReader::supportedImageFormats();
        for (const QByteArray &fmt : supported) {
            set.insert(QString::fromLatin1(fmt).toLower());
        }
        return set;
    }();
    return formats;
}

/**
 * @brief Checks whether a file info entry points to a supported image file.
 * @param info File info entry to inspect.
 * @return True if the entry is a supported image file, false otherwise.
 */
bool isImageFile(const QFileInfo &info)
{
    if (!info.exists() || info.isDir()) {
        return false;
    }
    return supportedImageFormats().contains(info.suffix().toLower());
}

/**
 * @brief Reads an image signature by downscaling to a small RGB buffer.
 * @param path Image file path to read.
 * @return Signature structure with validity and image data.
 */
ImageSignature readSignature(const QString &path)
{
    ImageSignature result;
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        return result;
    }
    image = image.convertToFormat(QImage::Format_RGB32);
    image = image.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    result.valid = true;
    result.image = image;
    return result;
}

/**
 * @brief Compares two image signatures using per-channel tolerance.
 * @param left Left image signature.
 * @param right Right image signature.
 * @param tolerance Maximum per-channel difference allowed.
 * @return True when signatures are within tolerance, false otherwise.
 */
bool compareSignatures(const ImageSignature &left, const ImageSignature &right, int tolerance)
{
    if (!left.valid || !right.valid) {
        return false;
    }
    if (left.image.size() != right.image.size()) {
        return false;
    }
    const int width = left.image.width();
    const int height = left.image.height();
    for (int y = 0; y < height; y += 1) {
        const QRgb *leftLine = reinterpret_cast<const QRgb *>(left.image.constScanLine(y));
        const QRgb *rightLine = reinterpret_cast<const QRgb *>(right.image.constScanLine(y));
        for (int x = 0; x < width; x += 1) {
            const QRgb leftPixel = leftLine[x];
            const QRgb rightPixel = rightLine[x];
            if (std::abs(qRed(leftPixel) - qRed(rightPixel)) > tolerance
                || std::abs(qGreen(leftPixel) - qGreen(rightPixel)) > tolerance
                || std::abs(qBlue(leftPixel) - qBlue(rightPixel)) > tolerance) {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief Builds a unique ghost entry identifier for missing items.
 * @param name File name for the ghost entry.
 * @param modified Modified timestamp in seconds.
 * @param sideTag Side tag string for the missing entry.
 * @param index Incrementing ghost index.
 * @return Unique ghost identifier string.
 */
QString ghostId(const QString &name, qint64 modified, const QString &sideTag, int index)
{
    return QString("ghost:%1:%2:%3:%4").arg(sideTag).arg(name).arg(modified).arg(index);
}

/**
 * @brief Builds a cached entry including optional image signature data.
 * @param info File info entry to cache.
 * @param enabled Whether comparison is enabled.
 * @param computeSignatures Whether to compute image signatures.
 * @return Cached entry structure.
 */
FolderCompareModel::CachedEntry buildCachedEntry(const QFileInfo &info, bool enabled, bool computeSignatures)
{
    FolderCompareModel::CachedEntry entry;
    entry.info = info;
    entry.isImage = isImageFile(info);
    entry.signatureValid = false;
    entry.signatureAttempted = false;
    if (enabled && computeSignatures && entry.isImage && !info.isDir()) {
        entry.signatureAttempted = true;
        ImageSignature signature = readSignature(info.absoluteFilePath());
        entry.signatureValid = signature.valid;
        if (signature.valid) {
            entry.signature = signature.image;
        }
    }
    return entry;
}

/**
 * @brief Checks whether a path is inside the given root path.
 * @param rootPath Root folder path.
 * @param path Path to test.
 * @return True when the path is inside the root, false otherwise.
 */
bool isPathInRoot(const QString &rootPath, const QString &path)
{
    if (rootPath.isEmpty() || path.isEmpty()) {
        return false;
    }
    const QDir root(rootPath);
    const QString relative = root.relativeFilePath(path);
    return !relative.startsWith("..");
}

/**
 * @brief Scans a folder and builds a cache of direct children entries.
 * @param rootPath Root folder path to scan.
 * @param enabled Whether comparison is enabled.
 * @param computeSignatures Whether to compute image signatures.
 * @return Cache mapping absolute paths to cached entries.
 */
QHash<QString, FolderCompareModel::CachedEntry> scanCache(const QString &rootPath, bool enabled, bool computeSignatures)
{
    QHash<QString, FolderCompareModel::CachedEntry> cache;
    if (rootPath.isEmpty()) {
        return cache;
    }
    QDir dir(rootPath);
    const QFileInfoList infoList = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::NoSort);
    cache.reserve(infoList.size());
    for (const QFileInfo &info : infoList) {
        const QString path = info.absoluteFilePath();
        cache.insert(path, buildCachedEntry(info, enabled, computeSignatures));
    }
    return cache;
}


/**
 * @brief Updates cache entries for a list of paths inside a root.
 * @param cache Cache to update in place.
 * @param paths Paths to refresh.
 * @param rootPath Root folder path used for filtering.
 * @param enabled Whether comparison is enabled.
 * @param computeSignatures Whether to compute image signatures.
 */
void updateCacheForPaths(QHash<QString, FolderCompareModel::CachedEntry> &cache,
                         const QStringList &paths,
                         const QString &rootPath,
                         bool enabled,
                         bool computeSignatures)
{
    if (paths.isEmpty()) {
        return;
    }
    for (const QString &path : paths) {
        if (!isPathInRoot(rootPath, path)) {
            continue;
        }
        const QFileInfo info(path);
        if (!info.exists()) {
            cache.remove(path);
            continue;
        }
        cache.insert(path, buildCachedEntry(info, enabled, computeSignatures));
    }
}

/**
 * @brief Compares cached entries and builds compare results for both sides.
 * @param leftCache Cache of left side entries.
 * @param rightCache Cache of right side entries.
 * @param enabled Whether comparison is enabled.
 * @return Result containing left and right compare entries.
 */
CompareResult compareCaches(const QHash<QString, FolderCompareModel::CachedEntry> &leftCache,
                            const QHash<QString, FolderCompareModel::CachedEntry> &rightCache,
                            bool enabled)
{
    CompareResult result;
    QVector<MatchEntry> leftImages;
    QVector<MatchEntry> rightImages;
    QVector<MatchEntry> leftOthers;
    QVector<MatchEntry> rightOthers;

    auto buildEntries = [&](const QHash<QString, FolderCompareModel::CachedEntry> &cache,
                            QVector<FolderCompareSideModel::CompareEntry> &entries,
                            QVector<MatchEntry> &images,
                            QVector<MatchEntry> &others) {
        entries.reserve(cache.size());
        images.reserve(cache.size());
        others.reserve(cache.size());

        for (auto it = cache.constBegin(); it != cache.constEnd(); ++it) {
            const FolderCompareModel::CachedEntry &cached = it.value();
            const QFileInfo info = cached.info;

            FolderCompareSideModel::CompareEntry entry;
            entry.fileName = info.fileName();
            entry.filePath = info.absoluteFilePath();
            entry.created = info.birthTime().isValid() ? info.birthTime() : info.lastModified();
            entry.modified = info.lastModified();
            entry.isDir = info.isDir();
            entry.isImage = cached.isImage;
            entry.isGhost = false;
            entry.status = FolderCompareSideModel::StatusNone;
            entry.id = entry.filePath;
            const int entryIndex = entries.size();
            entries.append(entry);

            if (!enabled) {
                continue;
            }

            MatchEntry matchEntry;
            matchEntry.entryIndex = entryIndex;
            matchEntry.name = entry.fileName;
            matchEntry.modified = entry.modified.toSecsSinceEpoch();
            matchEntry.isDir = entry.isDir;
            if (entry.isImage && !entry.isDir) {
                if (cached.signatureValid) {
                    matchEntry.signature.valid = true;
                    matchEntry.signature.image = cached.signature;
                }
                matchEntry.signatureAttempted = cached.signatureAttempted;
                images.append(matchEntry);
            } else {
                others.append(matchEntry);
            }
        }
    };

    buildEntries(leftCache, result.leftEntries, leftImages, leftOthers);
    buildEntries(rightCache, result.rightEntries, rightImages, rightOthers);

    if (!enabled) {
        return result;
    }

    const int tolerance = 16;
    const qint64 timeToleranceSeconds = 1;

    for (MatchEntry &left : leftImages) {
        if (left.matched) {
            continue;
        }
        for (MatchEntry &right : rightImages) {
            if (right.matched) {
                continue;
            }
            if (left.name != right.name) {
                continue;
            }
            if (!left.signature.valid || !right.signature.valid) {
                left.matched = true;
                right.matched = true;
                const bool attemptedBoth = left.signatureAttempted && right.signatureAttempted;
                const auto status = attemptedBoth ? FolderCompareSideModel::StatusDifferent
                                                  : FolderCompareSideModel::StatusPending;
                result.leftEntries[left.entryIndex].status = status;
                result.rightEntries[right.entryIndex].status = status;
                break;
            }
            const bool sameContent = compareSignatures(left.signature, right.signature, tolerance);
            left.matched = true;
            right.matched = true;
            if (sameContent && std::abs(left.modified - right.modified) <= timeToleranceSeconds) {
                result.leftEntries[left.entryIndex].status = FolderCompareSideModel::StatusIdentical;
                result.rightEntries[right.entryIndex].status = FolderCompareSideModel::StatusIdentical;
            } else {
                result.leftEntries[left.entryIndex].status = FolderCompareSideModel::StatusDifferent;
                result.rightEntries[right.entryIndex].status = FolderCompareSideModel::StatusDifferent;
            }
            break;
        }
    }

    for (MatchEntry &left : leftImages) {
        if (left.matched) {
            continue;
        }
        if (!left.signature.valid) {
            if (!left.signatureAttempted) {
                left.matched = true;
                result.leftEntries[left.entryIndex].status = FolderCompareSideModel::StatusPending;
            }
        }
    }

    for (MatchEntry &right : rightImages) {
        if (right.matched) {
            continue;
        }
        if (!right.signature.valid) {
            if (!right.signatureAttempted) {
                right.matched = true;
                result.rightEntries[right.entryIndex].status = FolderCompareSideModel::StatusPending;
            }
        }
    }

    for (MatchEntry &left : leftOthers) {
        if (left.matched) {
            continue;
        }
        for (MatchEntry &right : rightOthers) {
            if (right.matched) {
                continue;
            }
            if (left.name != right.name) {
                continue;
            }
            if (left.isDir != right.isDir) {
                continue;
            }
            left.matched = true;
            right.matched = true;
            const bool sameModified = std::abs(left.modified - right.modified) <= timeToleranceSeconds;
            const auto status = left.isDir || sameModified
                ? FolderCompareSideModel::StatusIdentical
                : FolderCompareSideModel::StatusDifferent;
            result.leftEntries[left.entryIndex].status = status;
            result.rightEntries[right.entryIndex].status = status;
            break;
        }
    }

    int ghostIndex = 0;

    for (const MatchEntry &left : leftImages) {
        if (left.matched) {
            continue;
        }
        FolderCompareSideModel::CompareEntry &leftEntry = result.leftEntries[left.entryIndex];
        leftEntry.status = FolderCompareSideModel::StatusMissing;

        FolderCompareSideModel::CompareEntry ghost;
        ghost.fileName = leftEntry.fileName;
        ghost.filePath = QString();
        ghost.otherSidePath = leftEntry.filePath;
        ghost.created = leftEntry.created;
        ghost.modified = leftEntry.modified;
        ghost.isDir = false;
        ghost.isImage = true;
        ghost.isGhost = true;
        ghost.status = FolderCompareSideModel::StatusMissing;
        ghost.id = ghostId(ghost.fileName, ghost.modified.toSecsSinceEpoch(), "right", ghostIndex);
        ghostIndex += 1;
        result.rightEntries.append(ghost);
    }

    for (const MatchEntry &right : rightImages) {
        if (right.matched) {
            continue;
        }
        FolderCompareSideModel::CompareEntry &rightEntry = result.rightEntries[right.entryIndex];
        rightEntry.status = FolderCompareSideModel::StatusMissing;

        FolderCompareSideModel::CompareEntry ghost;
        ghost.fileName = rightEntry.fileName;
        ghost.filePath = QString();
        ghost.otherSidePath = rightEntry.filePath;
        ghost.created = rightEntry.created;
        ghost.modified = rightEntry.modified;
        ghost.isDir = false;
        ghost.isImage = true;
        ghost.isGhost = true;
        ghost.status = FolderCompareSideModel::StatusMissing;
        ghost.id = ghostId(ghost.fileName, ghost.modified.toSecsSinceEpoch(), "left", ghostIndex);
        ghostIndex += 1;
        result.leftEntries.append(ghost);
    }

    for (const MatchEntry &left : leftOthers) {
        if (left.matched) {
            continue;
        }
        FolderCompareSideModel::CompareEntry &leftEntry = result.leftEntries[left.entryIndex];
        leftEntry.status = FolderCompareSideModel::StatusMissing;

        FolderCompareSideModel::CompareEntry ghost;
        ghost.fileName = leftEntry.fileName;
        ghost.filePath = QString();
        ghost.otherSidePath = leftEntry.filePath;
        ghost.created = leftEntry.created;
        ghost.modified = leftEntry.modified;
        ghost.isDir = leftEntry.isDir;
        ghost.isImage = false;
        ghost.isGhost = true;
        ghost.status = FolderCompareSideModel::StatusMissing;
        ghost.id = ghostId(ghost.fileName, ghost.modified.toSecsSinceEpoch(), "right", ghostIndex);
        ghostIndex += 1;
        result.rightEntries.append(ghost);
    }

    for (const MatchEntry &right : rightOthers) {
        if (right.matched) {
            continue;
        }
        FolderCompareSideModel::CompareEntry &rightEntry = result.rightEntries[right.entryIndex];
        rightEntry.status = FolderCompareSideModel::StatusMissing;

        FolderCompareSideModel::CompareEntry ghost;
        ghost.fileName = rightEntry.fileName;
        ghost.filePath = QString();
        ghost.otherSidePath = rightEntry.filePath;
        ghost.created = rightEntry.created;
        ghost.modified = rightEntry.modified;
        ghost.isDir = rightEntry.isDir;
        ghost.isImage = false;
        ghost.isGhost = true;
        ghost.status = FolderCompareSideModel::StatusMissing;
        ghost.id = ghostId(ghost.fileName, ghost.modified.toSecsSinceEpoch(), "left", ghostIndex);
        ghostIndex += 1;
        result.leftEntries.append(ghost);
    }

    return result;
}

} // namespace

/**
 * @brief Constructs the folder compare model and initializes side models.
 * @param parent Parent QObject for ownership.
 */
FolderCompareModel::FolderCompareModel(QObject *parent)
    : QObject(parent)
{
    m_leftModel = new FolderCompareSideModel(this, Left, this);
    m_rightModel = new FolderCompareSideModel(this, Right, this);
    m_leftModel->setHideIdentical(m_hideIdentical);
    m_rightModel->setHideIdentical(m_hideIdentical);
    const auto handleSideOperationChanged = [this]() {
        const bool hasPending = m_pendingWatcherFullRefresh
            || !m_pendingWatcherLeftPaths.isEmpty()
            || !m_pendingWatcherRightPaths.isEmpty();
        if (isCopyInProgress() || !hasPending) {
            return;
        }
        if (m_pendingWatcherFullRefresh) {
            m_pendingWatcherFullRefresh = false;
            m_pendingWatcherLeftPaths.clear();
            m_pendingWatcherRightPaths.clear();
            requestRefresh();
            return;
        }
        const QStringList leftPaths = QStringList(m_pendingWatcherLeftPaths.begin(), m_pendingWatcherLeftPaths.end());
        const QStringList rightPaths = QStringList(m_pendingWatcherRightPaths.begin(), m_pendingWatcherRightPaths.end());
        m_pendingWatcherLeftPaths.clear();
        m_pendingWatcherRightPaths.clear();
        refreshFiles(leftPaths, rightPaths);
    };
    connect(m_leftModel, &FolderCompareSideModel::copyInProgressChanged, this, handleSideOperationChanged);
    connect(m_rightModel, &FolderCompareSideModel::copyInProgressChanged, this, handleSideOperationChanged);
    connect(m_leftModel, &FolderCompareSideModel::trashInProgressChanged, this, handleSideOperationChanged);
    connect(m_rightModel, &FolderCompareSideModel::trashInProgressChanged, this, handleSideOperationChanged);

    m_refreshTimer.setInterval(150);
    m_refreshTimer.setSingleShot(true);
    connect(&m_refreshTimer, &QTimer::timeout, this, &FolderCompareModel::refresh);

    m_signatureTimer.setInterval(200);
    m_signatureTimer.setSingleShot(false);
    connect(&m_signatureTimer, &QTimer::timeout, this, [this]() {
        if (!m_signatureLoading) {
            m_signatureTimer.stop();
            return;
        }
        QHash<QString, CachedEntry> leftCache;
        QHash<QString, CachedEntry> rightCache;
        {
            QMutexLocker locker(&m_cacheMutex);
            leftCache = m_leftCache;
            rightCache = m_rightCache;
        }
        const CompareResult result = compareCaches(leftCache, rightCache, true);
        if (m_signaturePartial) {
            if (m_leftModel) {
                m_leftModel->updateBaseEntriesPartial(result.leftEntries, m_signatureAffectedNames, m_signatureAffectedLeftPaths);
            }
            if (m_rightModel) {
                m_rightModel->updateBaseEntriesPartial(result.rightEntries, m_signatureAffectedNames, m_signatureAffectedRightPaths);
            }
        } else {
            applyResult(result.leftEntries, result.rightEntries);
        }
    });

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path) {
        if (!m_enabled) {
            return;
        }
    });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (!m_enabled) {
            return;
        }
        if (isCopyInProgress()) {
            if (isPathInRoot(m_leftPath, path)) {
                m_pendingWatcherLeftPaths.insert(path);
            } else if (isPathInRoot(m_rightPath, path)) {
                m_pendingWatcherRightPaths.insert(path);
            } else {
                m_pendingWatcherFullRefresh = true;
            }
            return;
        }
        if (isPathInRoot(m_leftPath, path)) {
            refreshFilesInternal({path}, {});
        } else if (isPathInRoot(m_rightPath, path)) {
            refreshFilesInternal({}, {path});
        } else {
            requestRefresh();
        }
    });
}

/**
 * @brief Returns the left side model.
 * @return Left side model as an abstract item model.
 */
QAbstractItemModel *FolderCompareModel::leftModel() const
{
    return m_leftModel;
}

/**
 * @brief Returns the right side model.
 * @return Right side model as an abstract item model.
 */
QAbstractItemModel *FolderCompareModel::rightModel() const
{
    return m_rightModel;
}

/**
 * @brief Returns whether the compare model is loading entries.
 * @return True when loading is in progress, false otherwise.
 */
bool FolderCompareModel::loading() const
{
    return m_loading;
}

/**
 * @brief Returns whether identical items are hidden.
 * @return True when identical items are hidden, false otherwise.
 */
bool FolderCompareModel::hideIdentical() const
{
    return m_hideIdentical;
}

/**
 * @brief Returns whether comparison is enabled.
 * @return True when comparison is enabled, false otherwise.
 */
bool FolderCompareModel::enabled() const
{
    return m_enabled;
}

/**
 * @brief Sets whether identical items are hidden.
 * @param hide True to hide identical items, false otherwise.
 */
void FolderCompareModel::setHideIdentical(bool hide)
{
    if (m_hideIdentical == hide) {
        return;
    }
    m_hideIdentical = hide;
    if (m_leftModel) {
        m_leftModel->setHideIdentical(hide);
    }
    if (m_rightModel) {
        m_rightModel->setHideIdentical(hide);
    }
    emit hideIdenticalChanged();
}

/**
 * @brief Enables or disables comparison and refreshes watchers.
 * @param enabled True to enable comparison, false otherwise.
 */
void FolderCompareModel::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    emit enabledChanged();
    updateWatchers();
    requestRefresh();
}

/**
 * @brief Schedules a refresh to coalesce rapid updates.
 */
void FolderCompareModel::requestRefresh()
{
    if (!m_refreshTimer.isActive()) {
        m_refreshTimer.start();
    }
}

/**
 * @brief Sets the root path for the given side.
 * @param side Side enum value.
 * @param path Root folder path to set.
 */
void FolderCompareModel::setSidePath(FolderCompareModel::Side side, const QString &path)
{
    if (side == Left) {
        m_leftPath = path;
    } else {
        m_rightPath = path;
    }
}

/**
 * @brief Returns the root path for the given side.
 * @param side Side enum value.
 * @return Root folder path for the side.
 */
QString FolderCompareModel::sidePath(FolderCompareModel::Side side) const
{
    return side == Left ? m_leftPath : m_rightPath;
}

/**
 * @brief Refreshes both sides by scanning and comparing cached entries.
 */
void FolderCompareModel::refresh()
{
    cancelSignatureRefresh();
    setLoading(true);
    const int token = ++m_generation;
    const QString leftPath = m_leftPath;
    const QString rightPath = m_rightPath;
    const bool enabled = m_enabled;

    auto future = QtConcurrent::run([leftPath, rightPath, enabled]() {
        RefreshResult output;
        output.leftCache = scanCache(leftPath, enabled, false);
        output.rightCache = scanCache(rightPath, enabled, false);
        output.result = compareCaches(output.leftCache, output.rightCache, enabled);
        return output;
    });

    auto *watcher = new QFutureWatcher<RefreshResult>(this);
    connect(watcher, &QFutureWatcher<RefreshResult>::finished, this, [this, watcher, token, leftPath, rightPath]() {
        const RefreshResult output = watcher->result();
        watcher->deleteLater();
        if (token != m_generation) {
            return;
        }
        {
            QMutexLocker locker(&m_cacheMutex);
            m_leftCache = output.leftCache;
            m_rightCache = output.rightCache;
        }
        updateWatchers();
        applyResult(output.result.leftEntries, output.result.rightEntries);
        setLoading(false);
        if (m_enabled) {
            startSignatureRefresh(leftPath, rightPath, m_leftCache, m_rightCache);
        }
    });

    watcher->setFuture(future);
}

/**
 * @brief Refreshes compare data for specific paths on each side.
 * @param leftPaths Paths to refresh on the left side.
 * @param rightPaths Paths to refresh on the right side.
 */
void FolderCompareModel::refreshFiles(const QStringList &leftPaths, const QStringList &rightPaths)
{
    refreshFilesInternal(leftPaths, rightPaths);
}

/**
 * @brief Refreshes compare data for specific paths with cache reuse.
 * @param leftPaths Paths to refresh on the left side.
 * @param rightPaths Paths to refresh on the right side.
 */
void FolderCompareModel::refreshFilesInternal(const QStringList &leftPaths, const QStringList &rightPaths)
{
    if (m_leftPath.isEmpty() || m_rightPath.isEmpty()) {
        requestRefresh();
        return;
    }

    cancelSignatureRefresh();
    setLoading(true);
    const int token = ++m_generation;
    const QString leftPath = m_leftPath;
    const QString rightPath = m_rightPath;
    const QStringList leftPathsCopy = leftPaths;
    const QStringList rightPathsCopy = rightPaths;
    const bool enabled = m_enabled;
    const QHash<QString, CachedEntry> leftCache = m_leftCache;
    const QHash<QString, CachedEntry> rightCache = m_rightCache;

    auto future = QtConcurrent::run([leftPaths, rightPaths, leftPath, rightPath, enabled, leftCache, rightCache]() {
        RefreshResult output;
        output.leftCache = leftCache;
        output.rightCache = rightCache;

        if (output.leftCache.isEmpty()) {
            output.leftCache = scanCache(leftPath, enabled, false);
        } else {
            updateCacheForPaths(output.leftCache, leftPaths, leftPath, enabled, false);
        }

        if (output.rightCache.isEmpty()) {
            output.rightCache = scanCache(rightPath, enabled, false);
        } else {
            updateCacheForPaths(output.rightCache, rightPaths, rightPath, enabled, false);
        }

        output.result = compareCaches(output.leftCache, output.rightCache, enabled);
        return output;
    });

    auto *watcher = new QFutureWatcher<RefreshResult>(this);
    connect(watcher, &QFutureWatcher<RefreshResult>::finished, this, [this, watcher, token, leftPath, rightPath, leftPathsCopy, rightPathsCopy]() {
        const RefreshResult output = watcher->result();
        watcher->deleteLater();
        if (token != m_generation) {
            return;
        }
        {
            QMutexLocker locker(&m_cacheMutex);
            m_leftCache = output.leftCache;
            m_rightCache = output.rightCache;
        }
        updateWatchers();
        if (leftPathsCopy.isEmpty() && rightPathsCopy.isEmpty()) {
            applyResult(output.result.leftEntries, output.result.rightEntries);
        } else {
            QSet<QString> affectedNames;
            QSet<QString> affectedLeftPaths = QSet<QString>(leftPathsCopy.begin(), leftPathsCopy.end());
            QSet<QString> affectedRightPaths = QSet<QString>(rightPathsCopy.begin(), rightPathsCopy.end());

            for (const QString &path : leftPathsCopy) {
                const QString name = QFileInfo(path).fileName();
                if (!name.isEmpty()) {
                    affectedNames.insert(name);
                }
            }
            for (const QString &path : rightPathsCopy) {
                const QString name = QFileInfo(path).fileName();
                if (!name.isEmpty()) {
                    affectedNames.insert(name);
                }
            }
            for (const FolderCompareSideModel::CompareEntry &entry : output.result.leftEntries) {
                if (!entry.filePath.isEmpty() && affectedLeftPaths.contains(entry.filePath)) {
                    if (!entry.fileName.isEmpty()) {
                        affectedNames.insert(entry.fileName);
                    }
                }
            }
            for (const FolderCompareSideModel::CompareEntry &entry : output.result.rightEntries) {
                if (!entry.filePath.isEmpty() && affectedRightPaths.contains(entry.filePath)) {
                    if (!entry.fileName.isEmpty()) {
                        affectedNames.insert(entry.fileName);
                    }
                }
            }

            if (m_leftModel) {
                m_leftModel->updateBaseEntriesPartial(output.result.leftEntries, affectedNames, affectedLeftPaths);
            }
            if (m_rightModel) {
                m_rightModel->updateBaseEntriesPartial(output.result.rightEntries, affectedNames, affectedRightPaths);
            }
        }
        setLoading(false);
        if (m_enabled) {
            startSignatureRefresh(leftPath, rightPath, m_leftCache, m_rightCache, leftPathsCopy, rightPathsCopy);
        }
    });

    watcher->setFuture(future);
}

/**
 * @brief Updates file system watchers for current roots and files.
 */
void FolderCompareModel::updateWatchers()
{
    const QStringList currentDirs = m_watcher.directories();
    if (!currentDirs.isEmpty()) {
        m_watcher.removePaths(currentDirs);
    }
    const QStringList currentFiles = m_watcher.files();
    if (!currentFiles.isEmpty()) {
        m_watcher.removePaths(currentFiles);
    }

    if (!m_enabled) {
        return;
    }

    QStringList nextDirs;
    if (!m_leftPath.isEmpty()) {
        nextDirs.append(m_leftPath);
    }
    if (!m_rightPath.isEmpty() && m_rightPath != m_leftPath) {
        nextDirs.append(m_rightPath);
    }
    if (!nextDirs.isEmpty()) {
        m_watcher.addPaths(nextDirs);
    }

    QStringList nextFiles;
    {
        QMutexLocker locker(&m_cacheMutex);
        nextFiles.reserve(m_leftCache.size() + m_rightCache.size());
        for (auto it = m_leftCache.constBegin(); it != m_leftCache.constEnd(); ++it) {
            if (it.value().info.isFile()) {
                nextFiles.append(it.key());
            }
        }
        for (auto it = m_rightCache.constBegin(); it != m_rightCache.constEnd(); ++it) {
            if (it.value().info.isFile()) {
                nextFiles.append(it.key());
            }
        }
    }
    if (!nextFiles.isEmpty()) {
        m_watcher.addPaths(nextFiles);
    }
}

/**
 * @brief Updates the loading state and emits change notification.
 * @param loading True when loading is active, false otherwise.
 */
void FolderCompareModel::setLoading(bool loading)
{
    if (loading == m_loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

/**
 * @brief Reports whether any side is currently copying.
 * @return True when either side is copying, false otherwise.
 */
bool FolderCompareModel::isCopyInProgress() const
{
    const bool leftCopying = m_leftModel && m_leftModel->copyInProgress();
    const bool rightCopying = m_rightModel && m_rightModel->copyInProgress();
    const bool leftTrashing = m_leftModel && m_leftModel->trashInProgress();
    const bool rightTrashing = m_rightModel && m_rightModel->trashInProgress();
    return leftCopying || rightCopying || leftTrashing || rightTrashing;
}

/**
 * @brief Applies compare results to both side models.
 * @param left Compare entries for the left side.
 * @param right Compare entries for the right side.
 */
void FolderCompareModel::applyResult(const QVector<FolderCompareSideModel::CompareEntry> &left,
                                     const QVector<FolderCompareSideModel::CompareEntry> &right)
{
    if (m_leftModel) {
        m_leftModel->setBaseEntries(left);
    }
    if (m_rightModel) {
        m_rightModel->setBaseEntries(right);
    }
}

/**
 * @brief Cancels any ongoing signature refresh work.
 */
void FolderCompareModel::cancelSignatureRefresh()
{
    m_signatureGeneration.ref();
    m_signatureLoading = false;
    m_signatureTimer.stop();
}

/**
 * @brief Starts signature computation for image comparison.
 * @param leftPath Left root folder path.
 * @param rightPath Right root folder path.
 * @param leftCache Left cache snapshot.
 * @param rightCache Right cache snapshot.
 * @param leftPaths Optional left paths to refresh partially.
 * @param rightPaths Optional right paths to refresh partially.
 */
void FolderCompareModel::startSignatureRefresh(const QString &leftPath,
                                               const QString &rightPath,
                                               const QHash<QString, CachedEntry> &leftCache,
                                               const QHash<QString, CachedEntry> &rightCache,
                                               const QStringList &leftPaths,
                                               const QStringList &rightPaths)
{
    if (!m_enabled || m_signatureLoading) {
        return;
    }
    m_signatureLoading = true;
    const bool partial = !leftPaths.isEmpty() || !rightPaths.isEmpty();
    m_signaturePartial = partial;
    m_signatureAffectedNames.clear();
    m_signatureAffectedLeftPaths.clear();
    m_signatureAffectedRightPaths.clear();
    if (partial) {
        m_signatureAffectedLeftPaths = QSet<QString>(leftPaths.begin(), leftPaths.end());
        m_signatureAffectedRightPaths = QSet<QString>(rightPaths.begin(), rightPaths.end());
        for (const QString &path : leftPaths) {
            const QString name = QFileInfo(path).fileName();
            if (!name.isEmpty()) {
                m_signatureAffectedNames.insert(name);
            }
        }
        for (const QString &path : rightPaths) {
            const QString name = QFileInfo(path).fileName();
            if (!name.isEmpty()) {
                m_signatureAffectedNames.insert(name);
            }
        }
    }

    const int token = m_signatureGeneration.fetchAndAddRelaxed(1) + 1;
    if (!m_signatureTimer.isActive()) {
        m_signatureTimer.start();
    }

    QStringList pendingLeft;
    QStringList pendingRight;
    if (partial) {
        for (const QString &path : leftPaths) {
            auto it = leftCache.find(path);
            if (it == leftCache.end()) {
                continue;
            }
            const CachedEntry &entry = it.value();
            if (!entry.signatureValid && entry.isImage && !entry.info.isDir()) {
                pendingLeft.append(path);
            }
        }
        for (const QString &path : rightPaths) {
            auto it = rightCache.find(path);
            if (it == rightCache.end()) {
                continue;
            }
            const CachedEntry &entry = it.value();
            if (!entry.signatureValid && entry.isImage && !entry.info.isDir()) {
                pendingRight.append(path);
            }
        }
    } else {
        for (auto it = leftCache.constBegin(); it != leftCache.constEnd(); ++it) {
            const CachedEntry &entry = it.value();
            if (!entry.signatureValid && entry.isImage && !entry.info.isDir()) {
                pendingLeft.append(it.key());
            }
        }
        for (auto it = rightCache.constBegin(); it != rightCache.constEnd(); ++it) {
            const CachedEntry &entry = it.value();
            if (!entry.signatureValid && entry.isImage && !entry.info.isDir()) {
                pendingRight.append(it.key());
            }
        }
    }

    auto future = QtConcurrent::run([this, token, pendingLeft, pendingRight]() {
        auto applySignature = [this, token](const QString &path) {
            if (token != m_signatureGeneration.loadRelaxed()) {
                return false;
            }
            ImageSignature signature = readSignature(path);
            QMutexLocker locker(&m_cacheMutex);
            if (token != m_signatureGeneration.loadRelaxed()) {
                return false;
            }
            auto it = m_leftCache.find(path);
            if (it != m_leftCache.end()) {
                CachedEntry &entry = it.value();
                if (!entry.signatureValid && entry.isImage && !entry.info.isDir()) {
                    entry.signatureAttempted = true;
                    entry.signatureValid = signature.valid;
                    if (signature.valid) {
                        entry.signature = signature.image;
                    }
                }
            } else {
                auto itRight = m_rightCache.find(path);
                if (itRight != m_rightCache.end()) {
                    CachedEntry &entry = itRight.value();
                    if (!entry.signatureValid && entry.isImage && !entry.info.isDir()) {
                        entry.signatureAttempted = true;
                        entry.signatureValid = signature.valid;
                        if (signature.valid) {
                            entry.signature = signature.image;
                        }
                    }
                }
            }
            return true;
        };

        for (const QString &path : pendingLeft) {
            if (!applySignature(path)) {
                return;
            }
        }
        for (const QString &path : pendingRight) {
            if (!applySignature(path)) {
                return;
            }
        }
        QMetaObject::invokeMethod(this, [this, token]() {
            if (token != m_signatureGeneration.loadRelaxed()) {
                return;
            }
            m_signatureLoading = false;
            m_signatureTimer.stop();
            QHash<QString, CachedEntry> leftCache;
            QHash<QString, CachedEntry> rightCache;
            {
                QMutexLocker locker(&m_cacheMutex);
                leftCache = m_leftCache;
                rightCache = m_rightCache;
            }
            const CompareResult result = compareCaches(leftCache, rightCache, true);
            if (m_signaturePartial) {
                if (m_leftModel) {
                    m_leftModel->updateBaseEntriesPartial(result.leftEntries, m_signatureAffectedNames, m_signatureAffectedLeftPaths);
                }
                if (m_rightModel) {
                    m_rightModel->updateBaseEntriesPartial(result.rightEntries, m_signatureAffectedNames, m_signatureAffectedRightPaths);
                }
            } else {
                applyResult(result.leftEntries, result.rightEntries);
            }
        }, Qt::QueuedConnection);
    });

    auto *watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, this, [watcher]() {
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}
