#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QString>

namespace FolderEntryView {

struct EntryView {
    QString fileName;
    QString filePath;
    bool isFolder = false;
    bool isGhost = false;
    QDateTime createdTime;
    QDateTime modifiedTime;
    bool isImageFlag = false;
    bool isVideoFlag = false;
    qint64 byteSize = -1;
};

EntryView fromFileInfo(const QFileInfo &info, bool isImage, bool isVideo);

QString suffixForFileName(const QString &fileName);

bool matchesJunkExtension(const QString &suffix, const QStringList &junkExtensions);

bool matchesNameFilter(const QString &fileName, const QString &needle);

} // namespace FolderEntryView
