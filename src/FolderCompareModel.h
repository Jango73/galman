#pragma once

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHash>
#include <QImage>
#include <QSet>
#include <QTimer>
#include <QVector>
#include <QMutex>
#include <QAtomicInt>

#include <QtQml/qqml.h>

#include "FolderCompareSideModel.h"

class FolderCompareModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QAbstractItemModel *leftModel READ leftModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *rightModel READ rightModel CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(bool hideIdentical READ hideIdentical WRITE setHideIdentical NOTIFY hideIdenticalChanged)

public:
    enum Side {
        Left = 0,
        Right = 1
    };
    Q_ENUM(Side)

    struct CachedEntry {
        QFileInfo info;
        bool isImage = false;
        QImage signature;
        bool signatureValid = false;
        bool signatureAttempted = false;
    };

    explicit FolderCompareModel(QObject *parent = nullptr);

    QAbstractItemModel *leftModel() const;
    QAbstractItemModel *rightModel() const;
    bool enabled() const;
    void setEnabled(bool enabled);
    bool loading() const;
    bool hideIdentical() const;
    void setHideIdentical(bool hide);

    Q_INVOKABLE void refreshFiles(const QStringList &leftPaths, const QStringList &rightPaths);

signals:
    void enabledChanged();
    void loadingChanged();
    void hideIdenticalChanged();

private:
    friend class FolderCompareSideModel;

    bool isCopyInProgress() const;
    void requestRefresh();
    void updateWatchers();
    void startSignatureRefresh(const QString &leftPath,
                               const QString &rightPath,
                               const QHash<QString, CachedEntry> &leftCache,
                               const QHash<QString, CachedEntry> &rightCache,
                               const QStringList &leftPaths = {},
                               const QStringList &rightPaths = {});
    void cancelSignatureRefresh();
    void setSidePath(Side side, const QString &path);
    QString sidePath(Side side) const;
    void refresh();
    void refreshFilesInternal(const QStringList &leftPaths, const QStringList &rightPaths);
    void setLoading(bool loading);

    void applyResult(const QVector<FolderCompareSideModel::CompareEntry> &left,
                     const QVector<FolderCompareSideModel::CompareEntry> &right);

    FolderCompareSideModel *m_leftModel = nullptr;
    FolderCompareSideModel *m_rightModel = nullptr;
    QString m_leftPath;
    QString m_rightPath;
    bool m_enabled = false;
    bool m_pendingWatcherFullRefresh = false;
    QSet<QString> m_pendingWatcherLeftPaths;
    QSet<QString> m_pendingWatcherRightPaths;
    QAtomicInt m_signatureGeneration = 0;
    bool m_signatureLoading = false;
    QTimer m_signatureTimer;
    QMutex m_cacheMutex;
    bool m_loading = false;
    bool m_hideIdentical = false;
    QFileSystemWatcher m_watcher;
    QTimer m_refreshTimer;
    int m_generation = 0;
    QHash<QString, CachedEntry> m_leftCache;
    QHash<QString, CachedEntry> m_rightCache;
    bool m_signaturePartial = false;
    QSet<QString> m_signatureAffectedNames;
    QSet<QString> m_signatureAffectedLeftPaths;
    QSet<QString> m_signatureAffectedRightPaths;
};
