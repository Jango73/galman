#pragma once

#include <QAbstractListModel>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QVector>
#include <QVariantList>
#include <QSettings>

#include <QtQml/qqml.h>

class QThread;
class CopyWorker;

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
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QStringList selectedPaths READ selectedPaths NOTIFY selectedPathsChanged)
    Q_PROPERTY(bool selectedIsImage READ selectedIsImage NOTIFY selectedIsImageChanged)
    Q_PROPERTY(bool copyInProgress READ copyInProgress NOTIFY copyInProgressChanged)
    Q_PROPERTY(qreal copyProgress READ copyProgress NOTIFY copyProgressChanged)

public:
    enum SortKey {
        Name = 0,
        Extension,
        Created,
        Modified
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

    bool loading() const;
    QStringList selectedPaths() const;
    bool selectedIsImage() const;
    bool copyInProgress() const;
    qreal copyProgress() const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE void select(int row, bool multi);
    Q_INVOKABLE bool isSelected(int row) const;
    Q_INVOKABLE bool isDir(int row) const;
    Q_INVOKABLE QString pathForRow(int row) const;
    Q_INVOKABLE QString modifiedForRow(int row) const;
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
    Q_INVOKABLE void cancelCopy();
    Q_INVOKABLE QVariantMap moveSelectedToTrash();

signals:
    void rootPathChanged();
    void settingsKeyChanged();
    void nameFilterChanged();
    void sortKeyChanged();
    void sortOrderChanged();
    void showDirsFirstChanged();
    void loadingChanged();
    void selectedPathsChanged();
    void selectedIsImageChanged();
    void copyInProgressChanged();
    void copyProgressChanged();
    void copyFinished(QVariantMap result);

    void folderActivated(const QString &path);
    void fileActivated(const QString &path);

private:
    void setLoading(bool loading);
    void setCopyInProgress(bool inProgress);
    void updateCopyProgress(int completed, int total);
    void scheduleRefresh();
    void updateFileWatchers(const QVector<QFileInfo> &entries);
    void applyEntriesIncremental(const QVector<QFileInfo> &entries);
    void applyFilterAndSort(QVector<QFileInfo> &entries) const;
    void notifySelectionChanged();

    QString m_rootPath;
    QString m_settingsKey;
    QString m_nameFilter;
    SortKey m_sortKey = Name;
    Qt::SortOrder m_sortOrder = Qt::AscendingOrder;
    bool m_showDirsFirst = true;
    bool m_loading = false;
    QStringList m_selectedPaths;
    bool m_selectedIsImage = false;
    bool m_copyInProgress = false;
    int m_copyCompleted = 0;
    int m_copyTotal = 0;
    qreal m_copyProgress = 0.0;
    QThread *m_copyThread = nullptr;
    CopyWorker *m_copyWorker = nullptr;

    QVector<QFileInfo> m_entries;
    QFileSystemWatcher m_watcher;
    QTimer m_refreshTimer;
    int m_generation = 0;
    bool m_pendingWatcherRefresh = false;
};
