/************************************************************************\

    Galman - Picture gallery manager
    Copyright (C) 2026 Jango73

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

\************************************************************************/

#include "FavoritePairsManager.h"

#include <QDir>
#include <QSettings>

namespace {
constexpr char favoritesGroup[] = "favorites";
constexpr char favoritesArray[] = "pairs";
constexpr char leftPathKey[] = "leftPath";
constexpr char rightPathKey[] = "rightPath";
}

FavoritePairsManager::FavoritePairsManager(QObject *parent)
    : QObject(parent)
{
    loadFromSettings();
}

QVariantList FavoritePairsManager::favoritePairs() const
{
    QVariantList list;
    list.reserve(m_pairs.size());
    for (const FavoritePair &pair : m_pairs) {
        QVariantMap entry;
        entry.insert(QStringLiteral("leftPath"), pair.leftPath);
        entry.insert(QStringLiteral("rightPath"), pair.rightPath);
        list.append(entry);
    }
    return list;
}

bool FavoritePairsManager::addFavoritePair(const QString &leftPath, const QString &rightPath)
{
    FavoritePair pair{normalizePath(leftPath), normalizePath(rightPath)};
    if (!isValid(pair) || isDuplicate(pair)) {
        return false;
    }
    m_pairs.push_back(pair);
    saveToSettings();
    emit favoritePairsChanged();
    return true;
}

QString FavoritePairsManager::normalizePath(const QString &path)
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }
    return QDir::cleanPath(QDir::fromNativeSeparators(trimmed));
}

bool FavoritePairsManager::isDuplicate(const FavoritePair &pair) const
{
    for (const FavoritePair &existing : m_pairs) {
        if (existing.leftPath == pair.leftPath && existing.rightPath == pair.rightPath) {
            return true;
        }
    }
    return false;
}

bool FavoritePairsManager::isValid(const FavoritePair &pair) const
{
    return !(pair.leftPath.isEmpty() || pair.rightPath.isEmpty());
}

void FavoritePairsManager::loadFromSettings()
{
    m_pairs.clear();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Galman", "Galman");
    settings.beginGroup(QLatin1String(favoritesGroup));
    const int count = settings.beginReadArray(QLatin1String(favoritesArray));
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        FavoritePair pair;
        pair.leftPath = normalizePath(settings.value(QLatin1String(leftPathKey)).toString());
        pair.rightPath = normalizePath(settings.value(QLatin1String(rightPathKey)).toString());
        if (isValid(pair) && !isDuplicate(pair)) {
            m_pairs.push_back(pair);
        }
    }
    settings.endArray();
    settings.endGroup();
}

void FavoritePairsManager::saveToSettings() const
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Galman", "Galman");
    settings.beginGroup(QLatin1String(favoritesGroup));
    settings.remove(QLatin1String(""));
    settings.beginWriteArray(QLatin1String(favoritesArray));
    for (int i = 0; i < m_pairs.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue(QLatin1String(leftPathKey), m_pairs[i].leftPath);
        settings.setValue(QLatin1String(rightPathKey), m_pairs[i].rightPath);
    }
    settings.endArray();
    settings.endGroup();
    settings.sync();
}
