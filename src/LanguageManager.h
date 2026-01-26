#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QTranslator>

class LanguageManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList availableLanguages READ availableLanguages NOTIFY availableLanguagesChanged)
    Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY currentLanguageChanged)

public:
    explicit LanguageManager(QQmlApplicationEngine *engine, QObject *parent = nullptr);

    QVariantList availableLanguages() const;
    QString currentLanguage() const;

    Q_INVOKABLE void setLanguage(const QString &languageCode);

signals:
    void availableLanguagesChanged();
    void currentLanguageChanged();

private:
    void applyLanguage(const QString &languageCode, bool saveSetting);
    QString resolveLanguage(const QString &languageCode) const;
    void retranslateQml();
    void saveCurrentLanguage();
    QString loadSavedLanguage() const;

    QVariantList m_availableLanguages;
    QString m_currentLanguage;
    QTranslator m_translator;
    QQmlApplicationEngine *m_engine = nullptr;
};
