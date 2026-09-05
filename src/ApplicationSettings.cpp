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

#include "ApplicationSettings.h"
#include "ApplicationInfo.h"

#include <QSettings>

namespace
{
constexpr QSettings::Format settingsFormat = QSettings::IniFormat;
constexpr QSettings::Scope settingsScope = QSettings::UserScope;
} // namespace

ApplicationSettings::ApplicationSettings()
    : m_settings(std::make_unique<QSettings>(settingsFormat, settingsScope,
                                             ApplicationInfo::organizationName(),
                                             ApplicationInfo::applicationName()))
{
}

ApplicationSettings::~ApplicationSettings() = default;

QVariant ApplicationSettings::value(const QString &key, const QVariant &defaultValue) const
{
    return m_settings->value(key, defaultValue);
}

void ApplicationSettings::setValue(const QString &key, const QVariant &value)
{
    m_settings->setValue(key, value);
}

bool ApplicationSettings::contains(const QString &key) const
{
    return m_settings->contains(key);
}

void ApplicationSettings::remove(const QString &key)
{
    m_settings->remove(key);
}

void ApplicationSettings::clear()
{
    m_settings->clear();
}

void ApplicationSettings::sync()
{
    m_settings->sync();
}

QString ApplicationSettings::fileName() const
{
    return m_settings->fileName();
}

QString ApplicationSettings::group() const
{
    return m_settings->group();
}

QStringList ApplicationSettings::childGroups() const
{
    return m_settings->childGroups();
}

QStringList ApplicationSettings::childKeys() const
{
    return m_settings->childKeys();
}

QStringList ApplicationSettings::allKeys() const
{
    return m_settings->allKeys();
}

void ApplicationSettings::beginGroup(const QString &prefix)
{
    m_settings->beginGroup(prefix);
}

void ApplicationSettings::endGroup()
{
    m_settings->endGroup();
}

int ApplicationSettings::beginReadArray(const QString &prefix)
{
    return m_settings->beginReadArray(prefix);
}

void ApplicationSettings::beginWriteArray(const QString &prefix, int size)
{
    m_settings->beginWriteArray(prefix, size);
}

void ApplicationSettings::setArrayIndex(int index)
{
    m_settings->setArrayIndex(index);
}

void ApplicationSettings::endArray()
{
    m_settings->endArray();
}
