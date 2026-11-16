#include "Analytics.h"

#include <memory>

#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUuid>

#include "glog/logging.h"

namespace PastViewer {

struct Analytics::Impl
{
    QString token;
    QUrl endpoint;
    QString installationId;
    QNetworkAccessManager network;
};

Analytics::Analytics(const QString & token, const QUrl & host)
    : m_impl(std::make_unique<Impl>(token.trimmed(), host.resolved(QUrl("/i/v0/e/"))))
{
    if (m_impl->token.isEmpty())
        return;

    QSettings settings;
    m_impl->installationId = settings.value("analytics/installationId").toString();
    if (m_impl->installationId.isEmpty())
    {
        m_impl->installationId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        settings.setValue("analytics/installationId", m_impl->installationId);
        settings.sync();
        if (settings.status() != QSettings::NoError)
        {
            LOG(WARNING) << "Analytics disabled: installation ID could not be saved";
            m_impl->token.clear();
        }
    }
}

Analytics::~Analytics() = default;

void Analytics::Track(const QString & event, QJsonObject properties)
{
    if (m_impl->token.isEmpty() || event.isEmpty())
        return;

    properties.insert("distinct_id", m_impl->installationId);
    properties.insert("$process_person_profile", false);
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
