
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

#include "ScriptManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QTimer>

/**
 * @brief Constructs the script manager and starts watching the scripts folder.
 * @param parent Parent QObject for ownership.
 */
ScriptManager::ScriptManager(QObject *parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
    , m_refreshTimer(new QTimer(this))
{
    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(150);
    connect(m_refreshTimer, &QTimer::timeout, this, &ScriptManager::updateScripts);

    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        m_refreshTimer->start();
    });

    updateScripts();
}

/**
 * @brief Returns the current list of available scripts.
 * @return List of script entries with name and path.
 */
QVariantList ScriptManager::scripts() const
{
    return m_scripts;
}

/**
 * @brief Refreshes the script list by rescanning the scripts folder.
 */
void ScriptManager::refresh()
{
    updateScripts();
}

/**
 * @brief Updates the script list and watcher based on the resolved folder.
 */
void ScriptManager::updateScripts()
{
    const QString scriptsDir = resolveScriptsDir();
    if (scriptsDir != m_scriptsDir) {
        if (!m_scriptsDir.isEmpty()) {
            m_watcher->removePath(m_scriptsDir);
        }
        m_scriptsDir = scriptsDir;
        if (!m_scriptsDir.isEmpty()) {
            m_watcher->addPath(m_scriptsDir);
        }
    }

    QVariantList nextScripts;
    if (!m_scriptsDir.isEmpty()) {
        const QDir dir(m_scriptsDir);
        const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);

        nextScripts.reserve(entries.size());
        for (const QFileInfo &info : entries) {
            if (info.suffix().toLower() != "js") {
                continue;
            }
            QVariantMap entry;
            entry.insert("name", info.completeBaseName());
            entry.insert("path", info.absoluteFilePath());

            nextScripts.append(entry);
        }
    }

    if (m_scripts == nextScripts) {
        return;
    }

    m_scripts = nextScripts;
    emit scriptsChanged();
}

/**
 * @brief Resolves the scripts folder path relative to the working or app folder.
 * @return Scripts folder path, or an empty string if not found.
 */
QString ScriptManager::resolveScriptsDir() const
{
    const QString cwdDir = QDir::current().filePath("scripts");
    if (QFileInfo::exists(cwdDir) && QFileInfo(cwdDir).isDir()) {
        return cwdDir;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString appSibling = QDir(appDir).filePath("../scripts");
    if (QFileInfo::exists(appSibling) && QFileInfo(appSibling).isDir()) {
        return QDir(appSibling).absolutePath();
    }

    const QString appLocal = QDir(appDir).filePath("scripts");
    if (QFileInfo::exists(appLocal) && QFileInfo(appLocal).isDir()) {
        return appLocal;
    }

    return QString();
}
