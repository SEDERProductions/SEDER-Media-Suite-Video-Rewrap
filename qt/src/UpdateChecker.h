#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// Fetches a `latest.json` document over HTTPS and compares the advertised
// version with the running build. When an update is available, emits the
// download URL so the QML layer can show a banner that opens it in the
// system browser. Designed for opt-out (`--no-update-check` CLI flag or a
// persisted user setting).
class UpdateChecker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool checkOnLaunch READ checkOnLaunch WRITE setCheckOnLaunch NOTIFY checkOnLaunchChanged)
    Q_PROPERTY(bool updateAvailable READ updateAvailable NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString updateUrl READ updateUrl NOTIFY updateAvailableChanged)
    Q_PROPERTY(QString lastMessage READ lastMessage NOTIFY lastMessageChanged)

public:
    explicit UpdateChecker(QObject *parent = nullptr);

    bool checkOnLaunch() const;
    void setCheckOnLaunch(bool value);

    bool updateAvailable() const { return m_updateAvailable; }
    QString latestVersion() const { return m_latestVersion; }
    QString updateUrl() const { return m_updateUrl; }
    QString lastMessage() const { return m_lastMessage; }

    void setEndpointForTesting(const QString &url) { m_endpoint = url; }
    void evaluatePayloadForTesting(const QByteArray &body);

    Q_INVOKABLE void checkNow();
    Q_INVOKABLE void openUpdateUrl();
    Q_INVOKABLE void dismiss();

signals:
    void checkOnLaunchChanged();
    void updateAvailableChanged();
    void lastMessageChanged();

private:
    void onReplyFinished(QNetworkReply *reply);
    void applyEvaluation(const QByteArray &body);
    void setMessage(const QString &message);

    QNetworkAccessManager *m_network = nullptr;
    QString m_endpoint;
    QString m_currentVersion;
    bool m_updateAvailable = false;
    QString m_latestVersion;
    QString m_updateUrl;
    QString m_lastMessage;
};
