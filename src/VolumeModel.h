#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

#include <QtQml/qqml.h>

class VolumeModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Role {
        LabelRole = Qt::UserRole + 1,
        PathRole
    };
    Q_ENUM(Role)

    explicit VolumeModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString pathForIndex(int index) const;
    Q_INVOKABLE int indexForPath(const QString &path) const;

private:
    struct VolumeEntry {
        QString label;
        QString path;
    };

    void loadVolumes();
    void applyEntriesIncremental(const QVector<VolumeEntry> &entries);

    QVector<VolumeEntry> m_entries;
};
