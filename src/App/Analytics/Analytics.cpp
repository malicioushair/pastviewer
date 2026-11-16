#include "Analytics.h"

#include <memory>

#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSysInfo>
#include <QUuid>

#include "platform/DeviceInfo.h"

#include "glog/logging.h"

namespace PastViewer {

namespace {
constexpr auto CONSENT_KEY = "analytics/consentGranted";
}

struct Analytics::Impl
{
    QString token;
    QUrl endpoint;
    QString installationId;
    QNetworkAccessManager network;
    QString deviceModel;
};

Analytics::Analytics(const QString & token, const QUrl & host)
    : m_impl(std::make_unique<Impl>(token.trimmed(), host.resolved(QUrl("/i/v0/e/"))))
{
}

Analytics::~Analytics() = default;

bool Analytics::ConsentGranted() const
{
    return QSettings().value(CONSENT_KEY, false).toBool();
}

bool Analytics::ConsentDecided() const
{
    return QSettings().contains(CONSENT_KEY);
}

bool Analytics::AnalyticsAvailable() const
{
    return !m_impl->token.isEmpty();
}

void Analytics::SetConsentGranted(bool consentGranted)
{
    QSettings settings;
    settings.setValue(CONSENT_KEY, consentGranted);
    settings.sync();
    if (settings.status() != QSettings::NoError)
        LOG(WARNING) << "Failed to save analytics consent";
    emit consentChanged();
}

void Analytics::Track(const QString & event, QJsonObject properties) const
{
    if (false
        || m_impl->token.isEmpty()
        || event.isEmpty()
        || !ConsentGranted())
        return;

    if (m_impl->installationId.isEmpty())
    {
        QSettings settings;
        m_impl->installationId = settings.value("analytics/installationId").toString();
        if (m_impl->installationId.isEmpty())
        {
            const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            settings.setValue("analytics/installationId", id);
            settings.sync();
            if (settings.status() != QSettings::NoError)
            {
                LOG(WARNING) << "Analytics disabled: installation ID could not be saved";
                return;
            }
            m_impl->installationId = id;
        }
        m_impl->deviceModel = Platform::DeviceModel();
    }

    properties.insert("distinct_id", m_impl->installationId);
    properties.insert("$process_person_profile", false);
    properties.insert("$os", QSysInfo::productType());
    properties.insert("$os_version", QSysInfo::productVersion());
    if (!m_impl->deviceModel.isEmpty())
        properties.insert("$device_model", m_impl->deviceModel);
    const QJsonObject payload {
        { "api_key",    m_impl->token },
        { "event",      event         },
        { "properties", properties    }
    };

    QNetworkRequest request(m_impl->endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(10000);
    // Do not forward the project token to a redirect destination.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    auto * reply = m_impl->network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply] {
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || status < 200 || status >= 300)
            LOG(WARNING) << "Analytics capture failed; HTTP status:" << status;
        reply->deleteLater();
    });
}
} // namespace PastViewer
