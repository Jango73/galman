#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVariantList>

class ComfyPrerequisites : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)
    Q_PROPERTY(QVariantList nodeStatuses READ nodeStatuses NOTIFY nodeStatusesChanged)
    Q_PROPERTY(QVariantList modelRequirements READ modelRequirements NOTIFY modelRequirementsChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString statusInstalled READ statusInstalled CONSTANT)
    Q_PROPERTY(QString statusMissing READ statusMissing CONSTANT)
    Q_PROPERTY(QString statusUnknown READ statusUnknown CONSTANT)

public:
    explicit ComfyPrerequisites(QObject *parent = nullptr);
    ~ComfyPrerequisites() override;

    bool checking() const;
    QVariantList nodeStatuses() const;
    QVariantList modelRequirements() const;
    QString statusMessage() const;
    QString errorMessage() const;
    QString statusInstalled() const;
    QString statusMissing() const;
    QString statusUnknown() const;

    Q_INVOKABLE void refresh(const QString &serverUrl);
    Q_INVOKABLE void openManager(const QString &serverUrl);
    Q_INVOKABLE bool isLocalServer(const QString &serverUrl);

    static QVariantList buildUnknownStatuses();
    static QVariantList buildNodeStatuses(const QSet<QString> &availableTypes);
    static QVariantList buildUnknownModelStatuses();
    static QVariantList buildModelStatuses(const QString &serverUrl, const QString &modelsFolderPath);

signals:
    void checkingChanged();
    void nodeStatusesChanged();
    void modelRequirementsChanged();
    void statusMessageChanged();
    void errorMessageChanged();

private slots:
    void handleReplyFinished();

private:
    static QString normalizedServerUrl(const QString &serverUrl);
    static bool isLocalServerUrl(const QString &serverUrl);
    void setChecking(bool value);
    void setNodeStatuses(const QVariantList &value);
    void setModelRequirements(const QVariantList &value);
    void setStatusMessage(const QString &value);
    void setErrorMessage(const QString &value);

    QNetworkAccessManager *m_networkManager = nullptr;
    QPointer<QNetworkReply> m_pendingReply;
    bool m_checking = false;
    QVariantList m_nodeStatuses;
    QVariantList m_modelRequirements;
    QString m_statusMessage;
    QString m_errorMessage;
};
