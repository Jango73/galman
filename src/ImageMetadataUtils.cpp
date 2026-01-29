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

#include "ImageMetadataUtils.h"

#include <QImage>
#include <QImageReader>

namespace {
struct SignatureHashConstants {
    static constexpr quint64 hashOffset = 14695981039346656037ull;
    static constexpr quint64 hashPrime = 1099511628211ull;
};
} // namespace

namespace ImageMetadataUtils {

ImageSizeResult readImageSize(const QString &path)
{
    ImageSizeResult result;
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize size = reader.size();
    if (!size.isValid()) {
        return result;
    }
    result.valid = true;
    result.size = size;
    return result;
}

SignatureHashResult readSignatureHash(const QString &path, int signatureDimension)
{
    SignatureHashResult result;
    if (signatureDimension <= 0) {
        return result;
    }

    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull()) {
        return result;
    }

    image = image.convertToFormat(QImage::Format_RGB32);
    image = image.scaled(signatureDimension, signatureDimension, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    const qsizetype byteCount = image.sizeInBytes();
    if (byteCount <= 0) {
        return result;
    }

    const uchar *data = image.constBits();
    quint64 hash = SignatureHashConstants::hashOffset;
    for (qsizetype i = 0; i < byteCount; i += 1) {
        hash ^= static_cast<quint64>(data[i]);
        hash *= SignatureHashConstants::hashPrime;
    }

    result.valid = true;
    result.hash = hash;
    return result;
}

ImageSizeBatchResult readImageSizes(const QStringList &paths)
{
    ImageSizeBatchResult result;
    result.sizes.reserve(paths.size());
    result.attempted.reserve(paths.size());
    for (const QString &path : paths) {
        const ImageSizeResult sizeResult = readImageSize(path);
        result.attempted.insert(path);
        if (sizeResult.valid) {
            result.sizes.insert(path, sizeResult.size);
        }
    }
    return result;
}

SignatureHashBatchResult readSignatureHashes(const QStringList &paths, int signatureDimension)
{
    SignatureHashBatchResult result;
    result.hashes.reserve(paths.size());
    result.attempted.reserve(paths.size());
    for (const QString &path : paths) {
        const SignatureHashResult hashResult = readSignatureHash(path, signatureDimension);
        result.attempted.insert(path);
        if (hashResult.valid) {
            result.hashes.insert(path, hashResult.hash);
        }
    }
    return result;
}

} // namespace ImageMetadataUtils
