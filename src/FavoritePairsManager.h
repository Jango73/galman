#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVector>

class FavoritePairsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList favoritePairs READ favoritePairs NOTIFY favoritePairsChanged)

public:
    explicit FavoritePairsManager(QObject *parent = nullptr);

    QVariantList favoritePairs() const;

    Q_INVOKABLE bool addFavoritePair(const QString &leftPath, const QString &rightPath);

signals:
    void favoritePairsChanged();

private:
    struct FavoritePair {
        QString leftPath;
        QString rightPath;
    };

    QVector<FavoritePair> m_pairs;

    static QString normalizePath(const QString &path);
    bool isDuplicate(const FavoritePair &pair) const;
    bool isValid(const FavoritePair &pair) const;
    void loadFromSettings();
    void saveToSettings() const;
};
