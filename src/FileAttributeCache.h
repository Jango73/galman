#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QSize>
#include <QString>

class FileAttributeCache : public QObject
{
    Q_OBJECT

public:
    explicit FileAttributeCache(QObject *parent = nullptr);

    const QHash<QString, QSize> &imageSizes() const;
    const QHash<QString, quint64> &signatureHashes() const;
    const QHash<QString, QString> &videoThumbnails() const;

    void setImageSize(const QString &path, const QSize &size);
    void setSignatureHash(const QString &path, quint64 hash);
    void setVideoThumbnail(const QString &path, const QString &thumbnail);

    bool hasImageSize(const QString &path) const;
    bool hasSignatureHash(const QString &path) const;
    bool signatureHashForPath(const QString &path, quint64 *value) const;

    void pruneCaches(const QSet<QString> &currentPaths);

    bool imageSizeAttempted(const QString &path) const;
    void markImageSizeAttempted(const QString &path);
    bool signatureAttempted(const QString &path) const;
    void markSignatureAttempted(const QString &path);
    bool videoThumbnailAttempted(const QString &path) const;
    void markVideoThumbnailAttempted(const QString &path);

    void setImageSizeLoading(bool loading);
    bool imageSizeLoading() const;
    void setSignatureLoading(bool loading);
    bool signatureLoading() const;
    void setVideoThumbnailLoading(bool loading);
    bool videoThumbnailLoading() const;

    int imageSizeGeneration() const;
    void setImageSizeGeneration(int generation);
    int signatureGeneration() const;
    void setSignatureGeneration(int generation);
    int videoThumbnailGeneration() const;
    void setVideoThumbnailGeneration(int generation);

private:
    QHash<QString, QSize> m_imageSizes;
    QSet<QString> m_imageSizeAttempted;
    bool m_imageSizeLoading = false;
    int m_imageSizeGeneration = 0;
    QHash<QString, quint64> m_signatureHashes;
    QSet<QString> m_signatureAttempted;
    bool m_signatureLoading = false;
    int m_signatureGeneration = 0;
    QHash<QString, QString> m_videoThumbnails;
    QSet<QString> m_videoThumbnailAttempted;
    bool m_videoThumbnailLoading = false;
    int m_videoThumbnailGeneration = 0;
};
