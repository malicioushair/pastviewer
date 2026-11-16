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

public:
    Analytics(const QString & token, const QUrl & host);
    ~Analytics();

    Q_INVOKABLE void Track(const QString & event, QJsonObject properties = {}) const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace PastViewer
