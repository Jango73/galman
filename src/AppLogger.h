#pragma once

#include <QFile>
#include <QMutex>
#include <QString>
#include <QtGlobal>

class AppLogger
{
public:
    static AppLogger &instance();

    void initialize();

private:
    AppLogger() = default;
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);

    void rotateLogs(const QString &folderPath);
    void writeMessage(QtMsgType type, const QMessageLogContext &context, const QString &message);
    QString resolveLogFolderPath() const;
    QString logFilePath(const QString &folderPath, int archiveIndex = 0) const;
    QString messageTypeLabel(QtMsgType type) const;

    QMutex m_mutex;
    QFile m_file;
    bool m_initialized = false;
};
