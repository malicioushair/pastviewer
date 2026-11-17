#pragma once

#include <memory>

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QUrl>

namespace PastViewer {
class Analytics : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool consentGranted READ ConsentGranted WRITE SetConsentGranted NOTIFY consentChanged)
    Q_PROPERTY(bool consentDecided READ ConsentDecided NOTIFY consentChanged)
    Q_PROPERTY(bool analyticsAvailable READ AnalyticsAvailable CONSTANT)

public:
    Analytics(const QString & token, const QUrl & host);
    ~Analytics();

    bool ConsentGranted() const;
    bool ConsentDecided() const;
    bool AnalyticsAvailable() const;
    void SetConsentGranted(bool consentGranted);

    Q_INVOKABLE void Track(const QString & event, QJsonObject properties = {}) const;

signals:
    void consentChanged();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace PastViewer
