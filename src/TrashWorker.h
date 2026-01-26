#pragma once

#include <QObject>
#include <QAtomicInt>
#include <QVariantMap>
#include <QStringList>

class TrashWorker : public QObject
{
    Q_OBJECT

public:
    explicit TrashWorker(const QStringList &paths, QObject *parent = nullptr);

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
    QAtomicInt m_cancelled = 0;
};

