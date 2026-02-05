#pragma once

#include <QObject>
#include <QAtomicInt>
#include <QVariantMap>
#include <QString>
#include <QList>

struct CopyItem {
    QString sourcePath;
    QString targetPath;
    bool isDir = false;
};

class CopyWorker : public QObject
{
    Q_OBJECT

public:
    enum class OperationMode {
        Copy,
        Move
    };

    explicit CopyWorker(const QList<CopyItem> &items,
                        OperationMode mode = OperationMode::Copy,
                        QObject *parent = nullptr);

public slots:
    void start();
    void cancel();

signals:
    void progress(int completed, int total);
    void finished(QVariantMap result);

private:
    bool isCancelled() const;
    bool countEntries(const CopyItem &item, int &count, QString &firstError);
    bool copyItem(const CopyItem &item, int &completed, int total, QString &firstError);
    bool moveItem(const CopyItem &item, int &completed, int total, QString &firstError);
    bool removeSourcePath(const CopyItem &item, QString &firstError);
    bool copyDirRecursive(const QString &sourcePath,
                          const QString &targetPath,
                          int &completed,
                          int total,
                          QString &firstError);
    void tick(int &completed, int total);
    QString cancelledMessage() const;
    QString failedMessage() const;
    QString operationKey() const;

    QList<CopyItem> m_items;
    OperationMode m_mode = OperationMode::Copy;
    QAtomicInt m_cancelled = 0;
};
