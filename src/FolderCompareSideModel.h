#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "FileAttributeCache.h"
#include "FolderFilterSettings.h"
#include "FolderSelectionManager.h"
#include "FolderTransferController.h"

class FolderCompareModel;

class FolderCompareSideModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString rootPath READ rootPath WRITE setRootPath NOTIFY rootPathChanged)
    Q_PROPERTY(QString settingsKey READ settingsKey WRITE setSettingsKey NOTIFY settingsKeyChanged)
    Q_PROPERTY(QString nameFilter READ nameFilter WRITE setNameFilter NOTIFY nameFilterChanged)
    Q_PROPERTY(SortKey sortKey READ sortKey WRITE setSortKey NOTIFY sortKeyChanged)
    Q_PROPERTY(Qt::SortOrder sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
    Q_PROPERTY(bool showDirsFirst READ showDirsFirst WRITE setShowDirsFirst NOTIFY showDirsFirstChanged)
    Q_PROPERTY(bool hideJunkFiles READ hideJunkFiles WRITE setHideJunkFiles NOTIFY hideJunkFilesChanged)
    Q_PROPERTY(qint64 minimumByteSize READ minimumByteSize WRITE setMinimumByteSize NOTIFY minimumByteSizeChanged)
    Q_PROPERTY(qint64 maximumByteSize READ maximumByteSize WRITE setMaximumByteSize NOTIFY maximumByteSizeChanged)
    Q_PROPERTY(int minimumImageWidth READ minimumImageWidth WRITE setMinimumImageWidth NOTIFY minimumImageWidthChanged)
    Q_PROPERTY(int maximumImageWidth READ maximumImageWidth WRITE setMaximumImageWidth NOTIFY maximumImageWidthChanged)
    Q_PROPERTY(int minimumImageHeight READ minimumImageHeight WRITE setMinimumImageHeight NOTIFY minimumImageHeightChanged)
    Q_PROPERTY(int maximumImageHeight READ maximumImageHeight WRITE setMaximumImageHeight NOTIFY maximumImageHeightChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QStringList selectedPaths READ selectedPaths NOTIFY selectedPathsChanged)
    Q_PROPERTY(bool selectedIsImage READ selectedIsImage NOTIFY selectedIsImageChanged)
    Q_PROPERTY(bool selectedIsVideo READ selectedIsVideo NOTIFY selectedIsVideoChanged)
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

    enum CompareStatus {
        StatusNone = 0,
        StatusPending,
        StatusIdentical,
        StatusDifferent,
        StatusMissing
    };
    Q_ENUM(CompareStatus)

    enum Role {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole,
        OtherSidePathRole,
        IsDirRole,
        IsImageRole,
        IsVideoRole,
        ThumbnailPathRole,
        ThumbnailRevisionRole,
        SuffixRole,
        CreatedRole,
        ModifiedRole,
        SelectedRole,
        CompareStatusRole,
        GhostRole,
        IsNewerRole
    };

    struct CompareEntry {
        QString id;
        QString fileName;
        QString filePath;
        QString otherSidePath;
        QDateTime created;
        QDateTime modified;
        bool isDir = false;
        bool isImage = false;
        bool isVideo = false;
        bool isGhost = false;
        bool isNewer = false;
        CompareStatus status = StatusNone;
    };

    explicit FolderCompareSideModel(FolderCompareModel *compareModel, int side, QObject *parent = nullptr);

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
    bool hideJunkFiles() const;
    void setHideJunkFiles(bool enabled);
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
    bool selectedIsVideo() const;
    int selectedFileCount() const;
    qint64 selectedTotalBytes() const;
    bool copyInProgress() const;
    qreal copyProgress() const;
    bool trashInProgress() const;
    qreal trashProgress() const;
    void setHideIdentical(bool hide);

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
    Q_INVOKABLE int copyNameConflictCount(const QString &targetFolder) const;
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void goUp();
    Q_INVOKABLE void setSelection(const QVariantList &rows, bool additive);
    Q_INVOKABLE void setSelectionRange(int start, int end, bool additive);
    Q_INVOKABLE bool allSelected() const;
    Q_INVOKABLE QVariantList selectedRows() const;
    Q_INVOKABLE bool isImage(int row) const;
    Q_INVOKABLE bool isVideo(int row) const;
    Q_INVOKABLE bool isGhost(int row) const;
    Q_INVOKABLE int selectedCompareStatus() const;
    Q_INVOKABLE bool selectedIsGhost() const;
    Q_INVOKABLE bool selectedIsNewer() const;
    Q_INVOKABLE QVariantMap selectionStats() const;
    Q_INVOKABLE QVariantMap copySelectedTo(const QString &targetFolder);
    Q_INVOKABLE void startCopySelectedTo(const QString &targetFolder);
    Q_INVOKABLE void startMoveSelectedTo(const QString &targetFolder);
    Q_INVOKABLE void cancelCopy();
    Q_INVOKABLE QVariantMap moveSelectedToTrash();
    Q_INVOKABLE void startMoveSelectedToTrash();
    Q_INVOKABLE QVariantMap deleteSelectedPermanently();
    Q_INVOKABLE void startDeleteSelectedPermanently();
    Q_INVOKABLE void cancelTrash();
    Q_INVOKABLE QVariantMap renamePath(const QString &path, const QString &newName);
    Q_INVOKABLE bool hasGhostOnOtherSide(int row) const;
    Q_INVOKABLE bool createFolder(const QString &parentPath, const QString &folderName);

signals:
    void rootPathChanged();
    void settingsKeyChanged();
    void nameFilterChanged();
    void sortKeyChanged();
    void sortOrderChanged();
    void showDirsFirstChanged();
    void hideJunkFilesChanged();
    void minimumByteSizeChanged();
    void maximumByteSizeChanged();
    void minimumImageWidthChanged();
    void maximumImageWidthChanged();
    void minimumImageHeightChanged();
    void maximumImageHeightChanged();
    void loadingChanged();
    void selectedPathsChanged();
    void selectedIsImageChanged();
    void selectedIsVideoChanged();
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
    friend class FolderCompareModel;

    void setLoading(bool loading);
    void setBaseEntries(const QVector<CompareEntry> &entries);
    void updateBaseEntriesPartial(const QVector<CompareEntry> &entries,
                                  const QSet<QString> &affectedNames,
                                  const QSet<QString> &affectedPaths);
    void rebuildEntries();
    void applyEntriesIncremental(const QVector<CompareEntry> &entries);
    void applyFilterAndSort(QVector<CompareEntry> &entries) const;
    void requestImageSizeRefresh();
    void requestSignatureHashRefresh();
    void requestVideoThumbnailRefresh();
    void pruneCaches(const QVector<CompareEntry> &entries);
    bool byteSizeFiltersActive() const;
    bool imageSizeFiltersActive() const;
    bool signatureSortActive() const;
    void startTransferSelectedTo(const QString &targetFolder, bool moveItems);
    QVariantMap requestRemoval(bool moveToTrash);
    void startRemoval(bool moveToTrash);
    void notifySelectionChanged();
    void updateSelectionTotalsAsync();
    void setSelectionTotals(int fileCount, qint64 totalBytes);
    void rebuildSelectedPaths();
    void saveViewSettings() const;
    void restoreViewSettings();

    const CompareEntry *entryForRow(int row) const;
    CompareEntry *entryForRow(int row);
    QString keyForRow(int row) const;
    QStringList validSelectedPaths() const;

    FolderCompareModel *m_compareModel = nullptr;
    int m_side = 0;
    QString m_rootPath;
    QString m_settingsKey;
    FolderFilterSettings m_filterSettings;
    bool m_loading = false;
    bool m_hideIdentical = false;
    QString m_pendingSelectionId;
    FolderSelectionManager m_selectionManager;
    QStringList m_selectedPaths;
    bool m_selectedIsImage = false;
    bool m_selectedIsVideo = false;
    int m_selectedFileCount = 0;
    qint64 m_selectedTotalBytes = 0;
    int m_selectionTotalsGeneration = 0;
    FolderTransferController m_transferController;
    int m_copyExtraFailed = 0;
    QString m_copyExtraError;
    QVector<CompareEntry> m_baseEntries;
    QVector<CompareEntry> m_entries;
    FileAttributeCache m_attributeCache;
};
