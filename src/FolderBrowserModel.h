#pragma once

#include <QAbstractListModel>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHash>
#include <QSet>
#include <QSize>
#include <QTimer>
#include <QVector>
#include <QVariantList>
#include <QSettings>

#include <QtQml/qqml.h>

class QThread;
class CopyWorker;
class TrashWorker;

class FolderBrowserModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(QString settingsKey READ settingsKey WRITE setSettingsKey NOTIFY settingsKeyChanged)
    Q_PROPERTY(QString nameFilter READ nameFilter WRITE setNameFilter NOTIFY nameFilterChanged)
    Q_PROPERTY(SortKey sortKey READ sortKey WRITE setSortKey NOTIFY sortKeyChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_PROPERTY(bool showDirsFirst READ showDirsFirst WRITE setShowDirsFirst NOTIFY showDirsFirstChanged)
    Q_PROPERTY(qint64 minimumByteSize READ minimumByteSize WRITE setMinimumByteSize NOTIFY minimumByteSizeChanged)
    Q_PROPERTY(qint64 maximumByteSize READ maximumByteSize WRITE setMaximumByteSize NOTIFY maximumByteSizeChanged)
    Q_PROPERTY(int minimumImageWidth READ minimumImageWidth WRITE setMinimumImageWidth NOTIFY minimumImageWidthChanged)
    Q_PROPERTY(int maximumImageWidth READ maximumImageWidth WRITE setMaximumImageWidth NOTIFY maximumImageWidthChanged)
    Q_PROPERTY(int minimumImageHeight READ minimumImageHeight WRITE setMinimumImageHeight NOTIFY minimumImageHeightChanged)
    Q_PROPERTY(int maximumImageHeight READ maximumImageHeight WRITE setMaximumImageHeight NOTIFY maximumImageHeightChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QStringList selectedPaths READ selectedPaths NOTIFY selectedPathsChanged)
    Q_PROPERTY(bool selectedIsImage READ selectedIsImage NOTIFY selectedIsImageChanged)
    Q_PROPERTY(int selectedFileCount READ selectedFileCount NOTIFY selectedFileCountChanged)
    Q_PROPERTY(qint64 selectedTotalBytes READ selectedTotalBytes NOTIFY selectedTotalBytesChanged)
    Q_PROPERTY(bool copyInProgress READ copyInProgress NOTIFY copyInProgressChanged)
    Q_PROPERTY(qreal copyProgress READ copyProgress NOTIFY copyProgressChanged)
    Q_PROPERTY(bool trashInProgress READ trashInProgress NOTIFY trashInProgressChanged)
    Q_PROPERTY(qreal trashProgress READ trashProgress NOTIFY trashProgressChanged)

public:
    enum SortKey {
        Name = 0,
        Extension,
        Created,
        Modified,
        Signature
    };
    Q_ENUM(SortKey)

    enum Role {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole,
        IsDirRole,
        IsImageRole,
        SuffixRole,
        CreatedRole,
        ModifiedRole,
        SelectedRole,
        CompareStatusRole,
        GhostRole
    };

    explicit FolderBrowserModel(QObject *parent = nullptr);

    QString rootPath() const;
    void setRootPath(const QString &path);

    QString settingsKey() const;
    void setSettingsKey(const QString &key);

    QString nameFilter() const;
    void setNameFilter(const QString &filter);

    SortKey sortKey() const;
    void setSortKey(SortKey key);

    Qt::SortOrder sortOrder() const;
    void setSortOrder(Qt::SortOrder order);

    bool showDirsFirst() const;
    void setShowDirsFirst(bool enabled);
    qint64 minimumByteSize() const;
    void setMinimumByteSize(qint64 value);
    qint64 maximumByteSize() const;
    void setMaximumByteSize(qint64 value);
    int minimumImageWidth() const;
    void setMinimumImageWidth(int value);
    int maximumImageWidth() const;
    void setMaximumImageWidth(int value);
    int minimumImageHeight() const;
    void setMinimumImageHeight(int value);
    int maximumImageHeight() const;
    void setMaximumImageHeight(int value);

    bool loading() const;
    QStringList selectedPaths() const;
    bool selectedIsImage() const;
    int selectedFileCount() const;
    qint64 selectedTotalBytes() const;
    bool copyInProgress() const;
    qreal copyProgress() const;
    bool trashInProgress() const;
    qreal trashProgress() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE void select(int row, bool multi);
    Q_INVOKABLE bool isSelected(int row) const;
    Q_INVOKABLE bool isDir(int row) const;
    Q_INVOKABLE QString pathForRow(int row) const;
    Q_INVOKABLE int rowForPrefix(const QString &prefix, int startRow) const;
    Q_INVOKABLE QString modifiedForRow(int row) const;
    Q_INVOKABLE int copyNameConflictCount(const QString &targetDir) const;
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void goUp();
    Q_INVOKABLE void refreshFiles(const QStringList &paths);
    Q_INVOKABLE void setSelection(const QVariantList &rows, bool additive);
    Q_INVOKABLE void setSelectionRange(int start, int end, bool additive);
    Q_INVOKABLE bool allSelected() const;
    Q_INVOKABLE QVariantList selectedRows() const;
    Q_INVOKABLE bool isImage(int row) const;
    Q_INVOKABLE int selectedCompareStatus() const;
    Q_INVOKABLE bool selectedIsGhost() const;
    Q_INVOKABLE QVariantMap selectionStats() const;
    Q_INVOKABLE QVariantMap copySelectedTo(const QString &targetDir);
    Q_INVOKABLE void startCopySelectedTo(const QString &targetDir);
    Q_INVOKABLE void startMoveSelectedTo(const QString &targetDir);
    Q_INVOKABLE void cancelCopy();
    Q_INVOKABLE QVariantMap moveSelectedToTrash();
    Q_INVOKABLE void startMoveSelectedToTrash();
    Q_INVOKABLE QVariantMap deleteSelectedPermanently();
    Q_INVOKABLE void startDeleteSelectedPermanently();
    Q_INVOKABLE void cancelTrash();
    Q_INVOKABLE QVariantMap renamePath(const QString &path, const QString &newName);

signals:
    void rootPathChanged();
    void settingsKeyChanged();
    void nameFilterChanged();
    void sortKeyChanged();
    void sortOrderChanged();
    void showDirsFirstChanged();
    void minimumByteSizeChanged();
    void maximumByteSizeChanged();
    void minimumImageWidthChanged();
    void maximumImageWidthChanged();
    void minimumImageHeightChanged();
    void maximumImageHeightChanged();
    void loadingChanged();
    void selectedPathsChanged();
    void selectedIsImageChanged();
    void selectedFileCountChanged();
    void selectedTotalBytesChanged();
    void copyInProgressChanged();
    void copyProgressChanged();
    void copyFinished(QVariantMap result);
    void trashInProgressChanged();
    void trashProgressChanged();
    void trashFinished(QVariantMap result);

    void folderActivated(const QString &path);
    void fileActivated(const QString &path);

private:
    void setLoading(bool loading);
    void setCopyInProgress(bool inProgress);
    void updateCopyProgress(int completed, int total);
    void setTrashInProgress(bool inProgress);
    void updateTrashProgress(int completed, int total);
    void scheduleRefresh();
    void updateFileWatchers(const QVector<QFileInfo> &entries);
    void applyEntriesIncremental(const QVector<QFileInfo> &entries);
    void applyFilterAndSort(QVector<QFileInfo> &entries) const;
    void rebuildEntries();
    void pruneCaches(const QVector<QFileInfo> &entries);
    void requestImageSizeRefresh();
    void requestSignatureHashRefresh();
    bool byteSizeFiltersActive() const;
    bool imageSizeFiltersActive() const;
    bool signatureSortActive() const;
    void startTransferSelectedTo(const QString &targetDir, bool moveItems);
    QVariantMap requestRemoval(bool moveToTrash);
    void startRemoval(bool moveToTrash);
    void notifySelectionChanged();
    void updateSelectionTotalsAsync();
    void setSelectionTotals(int fileCount, qint64 totalBytes);

    QString m_rootPath;
    QString m_settingsKey;
    QString m_nameFilter;
    SortKey m_sortKey = Name;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    bool m_showDirsFirst = true;
    qint64 m_minimumByteSize = -1;
    qint64 m_maximumByteSize = -1;
    int m_minimumImageWidth = -1;
    int m_maximumImageWidth = -1;
    int m_minimumImageHeight = -1;
    int m_maximumImageHeight = -1;
    bool m_loading = false;
    QStringList m_selectedPaths;
    bool m_selectedIsImage = false;
    int m_selectedFileCount = 0;
    qint64 m_selectedTotalBytes = 0;
    int m_selectionTotalsGeneration = 0;
    bool m_copyInProgress = false;
    int m_copyCompleted = 0;
    int m_copyTotal = 0;
    qreal m_copyProgress = 0.0;
    QThread *m_copyThread = nullptr;
    CopyWorker *m_copyWorker = nullptr;
    bool m_trashInProgress = false;
    int m_trashCompleted = 0;
    int m_trashTotal = 0;
    qreal m_trashProgress = 0.0;
    QThread *m_trashThread = nullptr;
    TrashWorker *m_trashWorker = nullptr;

    QVector<QFileInfo> m_baseEntries;
    QVector<QFileInfo> m_entries;
    QHash<QString, QSize> m_imageSizeCache;
    QSet<QString> m_imageSizeAttempted;
    bool m_imageSizeLoading = false;
    int m_imageSizeGeneration = 0;
    QHash<QString, quint64> m_signatureHashCache;
    QSet<QString> m_signatureHashAttempted;
    bool m_signatureHashLoading = false;
    int m_signatureHashGeneration = 0;
    QFileSystemWatcher m_watcher;
    QTimer m_refreshTimer;
    int m_generation = 0;
    bool m_pendingWatcherRefresh = false;
};
