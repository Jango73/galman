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

#include "VideoThumbnailUtils.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QProcess>
#include <QStandardPaths>

namespace {

struct VideoThumbnailConstants {
    static constexpr int seekMilliseconds = 1000;
    static constexpr int ffmpegTimeoutMilliseconds = 15000;
    static constexpr int thumbnailWidthPixels = 512;
};

QString videoThumbnailCacheFolder()
{
    const QString baseCachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (baseCachePath.isEmpty()) {
        return QString();
    }
    QDir dir(baseCachePath);
    if (!dir.mkpath(QStringLiteral("video-thumbnails"))) {
        return QString();
    }
    return dir.filePath(QStringLiteral("video-thumbnails"));
}

QString videoThumbnailPathForSource(const QString &videoPath)
{
    const QString cacheFolder = videoThumbnailCacheFolder();
    if (cacheFolder.isEmpty()) {
        return QString();
    }
    const QFileInfo info(videoPath);
    const QString fingerprint = QStringLiteral("%1|%2|%3")
        .arg(info.absoluteFilePath())
        .arg(info.lastModified().toMSecsSinceEpoch())
        .arg(info.size());
    const QByteArray digest = QCryptographicHash::hash(fingerprint.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QDir(cacheFolder).filePath(QString::fromLatin1(digest) + QStringLiteral(".jpg"));
}

} // namespace

namespace VideoThumbnailUtils {

bool isVideoFile(const QFileInfo &info)
{
    if (!info.exists() || info.isDir()) {
        return false;
    }
    static QMimeDatabase mimeDatabase;
    const QMimeType mimeType = mimeDatabase.mimeTypeForFile(info, QMimeDatabase::MatchExtension);
    return mimeType.isValid() && mimeType.name().startsWith(QStringLiteral("video/"));
}

bool generateThumbnail(const QString &videoPath, QString *thumbnailPathOut)
{
    if (!thumbnailPathOut) {
        return false;
    }
    const QString thumbnailPath = videoThumbnailPathForSource(videoPath);
    if (thumbnailPath.isEmpty()) {
        return false;
    }
    if (QFileInfo::exists(thumbnailPath)) {
        *thumbnailPathOut = thumbnailPath;
        return true;
    }

    const QString widthArgument = QString::number(VideoThumbnailConstants::thumbnailWidthPixels);
    const QString scaleArgument = QStringLiteral("scale=%1:-2:force_original_aspect_ratio=decrease").arg(widthArgument);
    const QString seekSeconds = QString::number(
        static_cast<double>(VideoThumbnailConstants::seekMilliseconds) / 1000.0, 'f', 3);

    QProcess process;
    QStringList arguments = {
        QStringLiteral("-hide_banner"),
        QStringLiteral("-loglevel"), QStringLiteral("error"),
        QStringLiteral("-y"),
        QStringLiteral("-ss"), seekSeconds,
        QStringLiteral("-i"), videoPath,
        QStringLiteral("-frames:v"), QStringLiteral("1"),
        QStringLiteral("-vf"), scaleArgument,
        thumbnailPath
    };

    process.start(QStringLiteral("ffmpeg"), arguments);
    if (!process.waitForFinished(VideoThumbnailConstants::ffmpegTimeoutMilliseconds)
        || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0
        || !QFileInfo::exists(thumbnailPath)) {
        return false;
    }

    *thumbnailPathOut = thumbnailPath;
    return true;
}

} // namespace VideoThumbnailUtils
