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
    explicit CopyWorker(const QList<CopyItem> &items, QObject *parent = nullptr);

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
    bool copyDirRecursive(const QString &sourcePath,
                          const QString &targetPath,
                          int &completed,
                          int total,
                          QString &firstError);
    void tick(int &completed, int total);

    QList<CopyItem> m_items;
    QAtomicInt m_cancelled = 0;
};
