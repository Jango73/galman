#pragma once

#include <QObject>
#include <QAtomicInt>
#include <QVariantMap>
#include <QStringList>

class TrashWorker : public QObject
{
    Q_OBJECT

public:
    enum RemovalMode {
        MoveToTrash = 0,
        DeletePermanently
    };

    explicit TrashWorker(const QStringList &paths, RemovalMode mode, QObject *parent = nullptr);

public slots:
    void start();
    void cancel();

signals:
    void progress(int completed, int total);
    void finished(QVariantMap result);

private:
    bool isCancelled() const;
    void tick(int &completed, int total);

    QStringList m_paths;
    RemovalMode m_mode = MoveToTrash;
    QAtomicInt m_cancelled = 0;
};
