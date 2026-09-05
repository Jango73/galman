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

#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>

#include <memory>

class QSettings;

/**
 * @brief Central access point for persistent application settings.
 *
 * Wraps QSettings so that callers never configure format, scope,
 * organization or application names themselves. Those identities live
 * in a single place (ApplicationInfo.cpp).
 */
class ApplicationSettings
{
public:
    ApplicationSettings();
    ~ApplicationSettings();

    ApplicationSettings(const ApplicationSettings &) = delete;
    ApplicationSettings &operator=(const ApplicationSettings &) = delete;
    ApplicationSettings(ApplicationSettings &&) = delete;
    ApplicationSettings &operator=(ApplicationSettings &&) = delete;

    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    bool contains(const QString &key) const;
    void remove(const QString &key);
    void clear();
    void sync();
    QString fileName() const;
    QString group() const;
    QStringList childGroups() const;
    QStringList childKeys() const;
    QStringList allKeys() const;
    void beginGroup(const QString &prefix);
    void endGroup();
    int beginReadArray(const QString &prefix);
    void beginWriteArray(const QString &prefix, int size = -1);
    void setArrayIndex(int index);
    void endArray();

private:
    std::unique_ptr<QSettings> m_settings;
};
