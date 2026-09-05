#pragma once

#include <QObject>
#include <QThread>
#include <QVariantMap>

#include "CopyWorker.h"
#include "TrashWorker.h"

class FolderTransferController : public QObject
{
    Q_OBJECT

public:
    explicit FolderTransferController(QObject *parent = nullptr);
    ~FolderTransferController() override;

    bool copyInProgress() const;
    qreal copyProgress() const;
    bool trashInProgress() const;
    qreal trashProgress() const;

    QVariantMap copySelectedPaths(const QStringList &selectedPaths, const QString &targetFolder, const QString &context);
    void startTransferPaths(const QStringList &selectedPaths,
                            const QString &targetFolder,
                            bool moveItems,
                            int ghostCount,
                            const QString &ghostError);
    void startRemovalPaths(const QStringList &selectedPaths, bool moveToTrash);
    QVariantMap requestRemovalPaths(const QStringList &selectedPaths, bool moveToTrash);
    void cancelCopy();
    void cancelTrash();

    static int countNameConflicts(const QStringList &selectedPaths, const QString &targetFolder);
    static QVariantMap targetFolderError();
    static QVariantMap noSelectionError();
    static QVariantMap nothingToDeleteError();

signals:
    void copyInProgressChanged();
    void copyProgressChanged();
    void copyFinished(const QVariantMap &result);
    void trashInProgressChanged();
    void trashProgressChanged();
    void trashFinished(const QVariantMap &result);

private:
    void setCopyInProgress(bool inProgress);
    void updateCopyProgress(int completed, int total);
    void setTrashInProgress(bool inProgress);
    void updateTrashProgress(int completed, int total);

    static constexpr int emptyCount = 0;
    static constexpr qreal zeroProgress = 0.0;

    bool m_copyInProgress = false;
    int m_copyCompleted = 0;
    int m_copyTotal = 0;
    qreal m_copyProgress = zeroProgress;
    QThread *m_copyThread = nullptr;
    CopyWorker *m_copyWorker = nullptr;
    bool m_trashInProgress = false;
    int m_trashCompleted = 0;
    int m_trashTotal = 0;
    qreal m_trashProgress = zeroProgress;
    QThread *m_trashThread = nullptr;
    TrashWorker *m_trashWorker = nullptr;
};
