#pragma once

#include <QFileInfo>
#include <QString>

namespace VideoThumbnailUtils {

bool isVideoFile(const QFileInfo &info);
QString thumbnailPathForSource(const QString &videoPath);
bool generateThumbnail(const QString &videoPath, QString *thumbnailPathOut);

} // namespace VideoThumbnailUtils
