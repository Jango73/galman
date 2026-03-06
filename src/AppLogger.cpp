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

#include "AppLogger.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QTextStream>
#include <QDebug>
#include <cstdlib>
#include <cstdio>

namespace {
struct AppLoggerConstants {
    static constexpr int archiveCount = 9; // galman.log + galman.log.1..9 => 10 files
};
} // namespace

AppLogger &AppLogger::instance()
{
    static AppLogger logger;
    return logger;
}

void AppLogger::initialize()
{
    QString currentLogFilePath;
    {
        QMutexLocker locker(&m_mutex);
        if (m_initialized) {
            return;
        }

        const QString logFolderPath = resolveLogFolderPath();
        QDir().mkpath(logFolderPath);
        rotateLogs(logFolderPath);

        m_file.setFileName(logFilePath(logFolderPath));
        m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        currentLogFilePath = m_file.fileName();

        qInstallMessageHandler(AppLogger::messageHandler);
        m_initialized = true;
    }

    qInfo() << "AppLogger initialized. Log file:" << currentLogFilePath;
}

void AppLogger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    AppLogger::instance().writeMessage(type, context, message);
}

void AppLogger::rotateLogs(const QString &folderPath)
{
    const QString oldestPath = logFilePath(folderPath, AppLoggerConstants::archiveCount);
    if (QFileInfo::exists(oldestPath)) {
        QFile::remove(oldestPath);
    }

    for (int index = AppLoggerConstants::archiveCount - 1; index >= 1; --index) {
        const QString sourcePath = logFilePath(folderPath, index);
        if (!QFileInfo::exists(sourcePath)) {
            continue;
        }
        QFile::rename(sourcePath, logFilePath(folderPath, index + 1));
    }

    const QString currentPath = logFilePath(folderPath);
    if (QFileInfo::exists(currentPath)) {
        QFile::rename(currentPath, logFilePath(folderPath, 1));
    }
}

void AppLogger::writeMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QMutexLocker locker(&m_mutex);

    const QString timeText = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    const QString category = QString::fromUtf8(context.category ? context.category : "");
    const QString file = QString::fromUtf8(context.file ? context.file : "");
    const QString function = QString::fromUtf8(context.function ? context.function : "");
    const QString lineText = context.line > 0 ? QString::number(context.line) : QString("-");
    const QString formatted = QString("[%1] [%2] [%3] %4 (%5:%6, %7)\n")
                                  .arg(timeText,
                                       messageTypeLabel(type),
                                       category.isEmpty() ? QString("general") : category,
                                       message,
                                       file.isEmpty() ? QString("-") : file,
                                       lineText,
                                       function.isEmpty() ? QString("-") : function);

    if (m_file.isOpen()) {
        QTextStream output(&m_file);
        output << formatted;
        output.flush();
    }

    std::fputs(formatted.toUtf8().constData(), stderr);
    std::fflush(stderr);

    if (type == QtFatalMsg) {
        std::abort();
    }
}

QString AppLogger::resolveLogFolderPath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (path.isEmpty()) {
        path = QDir::home().filePath(".cache/Galman");
    }
    return path;
}

QString AppLogger::logFilePath(const QString &folderPath, int archiveIndex) const
{
    const QString basePath = QDir(folderPath).filePath("galman.log");
    if (archiveIndex <= 0) {
        return basePath;
    }
    return basePath + "." + QString::number(archiveIndex);
}

QString AppLogger::messageTypeLabel(QtMsgType type) const
{
    switch (type) {
    case QtDebugMsg:
        return "DEBUG";
    case QtInfoMsg:
        return "INFO";
    case QtWarningMsg:
        return "WARN";
    case QtCriticalMsg:
        return "ERROR";
    case QtFatalMsg:
        return "FATAL";
    }
    return "UNKNOWN";
}
