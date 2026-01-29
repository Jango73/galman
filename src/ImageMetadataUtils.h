#pragma once

#include <QHash>
#include <QSet>
#include <QSize>
#include <QString>
#include <QStringList>

namespace ImageMetadataUtils {

struct ImageSizeResult {
    bool valid = false;
    QSize size;
};

struct SignatureHashResult {
    bool valid = false;
    quint64 hash = 0;
};

struct ImageSizeBatchResult {
    QHash<QString, QSize> sizes;
    QSet<QString> attempted;
};

struct SignatureHashBatchResult {
    QHash<QString, quint64> hashes;
    QSet<QString> attempted;
};

ImageSizeResult readImageSize(const QString &path);
SignatureHashResult readSignatureHash(const QString &path, int signatureDimension);
ImageSizeBatchResult readImageSizes(const QStringList &paths);
SignatureHashBatchResult readSignatureHashes(const QStringList &paths, int signatureDimension);

} // namespace ImageMetadataUtils
