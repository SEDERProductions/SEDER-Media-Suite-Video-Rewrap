#include "UpdateChecker.h"

#include "RustBridge.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>

namespace {
constexpr auto kCheckOnLaunchKey = "update/checkOnLaunch";
constexpr auto kDefaultEndpoint =
    "https://github.com/SEDERProductions/SEDER-Media-Suite-Video-Rewrap/releases/latest/download/latest.json";
}

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_endpoint(QString::fromLatin1(kDefaultEndpoint))
    , m_currentVersion(QCoreApplication::applicationVersion())
{
    connect(m_network, &QNetworkAccessManager::finished, this, &UpdateChecker::onReplyFinished);
}

bool UpdateChecker::checkOnLaunch() const
{
    return QSettings().value(kCheckOnLaunchKey, true).toBool();
}

void UpdateChecker::setCheckOnLaunch(bool value)
{
    QSettings settings;
    if (settings.value(kCheckOnLaunchKey, true).toBool() == value) return;
    settings.setValue(kCheckOnLaunchKey, value);
    emit checkOnLaunchChanged();
}

void UpdateChecker::checkNow()
{
    if (m_endpoint.isEmpty()) {
        setMessage(tr("No update endpoint configured."));
        return;
    }
    if (m_currentVersion.isEmpty()) {
        setMessage(tr("Current application version is unknown; skipping update check."));
        return;
    }
    QNetworkRequest request{QUrl(m_endpoint)};
    request.setRawHeader("User-Agent",
        QStringLiteral("seder-video-rewrap/%1").arg(m_currentVersion).toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_network->get(request);
    setMessage(tr("Checking for updates..."));
}

void UpdateChecker::openUpdateUrl()
{
    if (m_updateUrl.isEmpty()) return;
    QDesktopServices::openUrl(QUrl(m_updateUrl));
}

void UpdateChecker::dismiss()
{
    if (!m_updateAvailable) return;
    m_updateAvailable = false;
    emit updateAvailableChanged();
}

void UpdateChecker::onReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        setMessage(tr("Update check failed: %1").arg(reply->errorString()));
        return;
    }
    applyEvaluation(reply->readAll());
}

void UpdateChecker::evaluatePayloadForTesting(const QByteArray &body)
{
    applyEvaluation(body);
}

void UpdateChecker::applyEvaluation(const QByteArray &body)
{
    const QJsonObject result = RustBridge::evaluateUpdate(QString::fromUtf8(body), m_currentVersion);
    if (!result.value("ok").toBool()) {
        setMessage(tr("Update check failed: %1").arg(result.value("error").toString()));
        return;
    }
    const QJsonObject update = result.value("update").toObject();
    const bool available = update.value("update_available").toBool();
    const QString url = update.value("url").toString();
    const QJsonObject latestObj = update.value("latest").toObject();
    const QString latestStr = QStringLiteral("%1.%2.%3")
        .arg(latestObj.value("major").toInt())
        .arg(latestObj.value("minor").toInt())
        .arg(latestObj.value("patch").toInt());

    const bool changed = available != m_updateAvailable
        || latestStr != m_latestVersion
        || url != m_updateUrl;

    m_updateAvailable = available;
    m_latestVersion = latestStr;
    m_updateUrl = url;
    setMessage(update.value("message").toString());
    if (changed) {
        emit updateAvailableChanged();
    }
}

void UpdateChecker::setMessage(const QString &message)
{
    if (m_lastMessage == message) return;
    m_lastMessage = message;
    emit lastMessageChanged();
}
