
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

#include "LanguageManager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QSettings>

namespace
{
struct LanguageEntry {
    QString code;
    QString name;
};

constexpr int languageCodeLength = 2;
const char settingsGroup[] = "language";
const char settingsKey[] = "current";

QVariantMap toVariantMap(const LanguageEntry &entry)
{
    QVariantMap map;
    map.insert("code", entry.code);
    map.insert("name", entry.name);
    return map;
}

QString normalizeLanguageCode(const QString &languageCode)
{
    return languageCode.left(languageCodeLength).toLower();
}

QList<LanguageEntry> buildAvailableLanguageEntries()
{
    return {
        {QStringLiteral("en"), QStringLiteral("English")},
        {QStringLiteral("fr"), QStringLiteral("Français")},
        {QStringLiteral("es"), QStringLiteral("Español")},
        {QStringLiteral("it"), QStringLiteral("Italiano")},
        {QStringLiteral("ru"), QStringLiteral("Русский")},
        {QStringLiteral("nl"), QStringLiteral("Nederlands")},
        {QStringLiteral("zh"), QStringLiteral("简体中文")},
    };
}
} // namespace

LanguageManager::LanguageManager(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    const QList<LanguageEntry> entries = buildAvailableLanguageEntries();
    m_availableLanguages.reserve(entries.size());
    for (const LanguageEntry &entry : entries) {
        m_availableLanguages.append(toVariantMap(entry));
    }

    const QString savedLanguage = loadSavedLanguage();
    QString selectedLanguage = resolveLanguage(savedLanguage);
    if (selectedLanguage.isEmpty()) {
        const QString systemLanguage = QLocale::system().name();
        selectedLanguage = resolveLanguage(systemLanguage);
    }
    if (selectedLanguage.isEmpty()) {
        selectedLanguage = QStringLiteral("en");
    }

    applyLanguage(selectedLanguage, false);
}

QVariantList LanguageManager::availableLanguages() const
{
    return m_availableLanguages;
}

QString LanguageManager::currentLanguage() const
{
    return m_currentLanguage;
}

void LanguageManager::setLanguage(const QString &languageCode)
{
    const QString resolvedLanguage = resolveLanguage(languageCode);
    if (resolvedLanguage.isEmpty() || resolvedLanguage == m_currentLanguage) {
        return;
    }
    applyLanguage(resolvedLanguage, true);
}

void LanguageManager::applyLanguage(const QString &languageCode, bool saveSetting)
{
    QCoreApplication::removeTranslator(&m_translator);

    if (languageCode != QStringLiteral("en")) {
        const QString translationPath = QString(":/i18n/galman_%1.qm").arg(languageCode);
        if (m_translator.load(translationPath)) {
            QCoreApplication::installTranslator(&m_translator);
        }
    }

    if (m_currentLanguage != languageCode) {
        m_currentLanguage = languageCode;
        emit currentLanguageChanged();
    }

    if (saveSetting) {
        saveCurrentLanguage();
    }

    retranslateQml();
}

QString LanguageManager::resolveLanguage(const QString &languageCode) const
{
    if (languageCode.isEmpty()) {
        return QString();
    }
    const QString normalized = normalizeLanguageCode(languageCode);
    for (const QVariant &variant : m_availableLanguages) {
        const QVariantMap entry = variant.toMap();
        if (entry.value("code").toString() == normalized) {
            return normalized;
        }
    }
    return QString();
}

void LanguageManager::retranslateQml()
{
    if (m_engine) {
        m_engine->retranslate();
    }
}

void LanguageManager::saveCurrentLanguage()
{
    QSettings settings;
    settings.beginGroup(QLatin1String(settingsGroup));
    settings.setValue(QLatin1String(settingsKey), m_currentLanguage);
}

QString LanguageManager::loadSavedLanguage() const
{
    QSettings settings;
    settings.beginGroup(QLatin1String(settingsGroup));
    return settings.value(QLatin1String(settingsKey)).toString();
}
