#pragma once

#include <QFileInfo>
#include <QString>

namespace VideoThumbnailUtils {

bool isVideoFile(const QFileInfo &info);
bool generateThumbnail(const QString &videoPath, QString *thumbnailPathOut);

} // namespace VideoThumbnailUtils
