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

#include <QObject>
#include <QString>

#include <QtQml/qqmlregistration.h>

class ImageAutoAdjuster : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)
    Q_PROPERTY(bool autoEnabled READ autoEnabled WRITE setAutoEnabled NOTIFY autoEnabledChanged)
    Q_PROPERTY(double brightnessValue READ brightnessValue NOTIFY brightnessValueChanged)
    Q_PROPERTY(double contrastValue READ contrastValue NOTIFY contrastValueChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit ImageAutoAdjuster(QObject *parent = nullptr);

    QString imagePath() const;
    void setImagePath(const QString &imagePath);

    bool autoEnabled() const;
    void setAutoEnabled(bool autoEnabled);

    double brightnessValue() const;
    double contrastValue() const;
    bool busy() const;

    Q_INVOKABLE void refresh();

signals:
    void imagePathChanged();
    void autoEnabledChanged();
    void brightnessValueChanged();
    void contrastValueChanged();
    void busyChanged();

private:
    void requestAdjustment();

    QString m_imagePath;
    bool m_autoEnabled;
    double m_brightnessValue;
    double m_contrastValue;
    bool m_busy;
    quint64 m_requestToken;
};
