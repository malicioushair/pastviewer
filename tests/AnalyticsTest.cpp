#include <gtest/gtest.h>

#include <memory>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSettings>
#include <QSysInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QUuid>

#include "App/Analytics/Analytics.h"

namespace {
class AnalyticsTest : public testing::Test
{
protected:
    void SetUp() override
    {
        if (!QCoreApplication::instance())
            app = std::make_unique<QCoreApplication>(argc, argv);
        oldOrganization = QCoreApplication::organizationName();
        oldApplication = QCoreApplication::applicationName();
        oldFormat = QSettings::defaultFormat();
        QDir settingsDirectory = QFileInfo(QSettings(QSettings::IniFormat, QSettings::UserScope, "PastViewerAnalyticsTests", "path-probe").fileName()).dir();
        settingsDirectory.cdUp();
        oldSettingsPath = settingsDirectory.path();
        QCoreApplication::setOrganizationName("PastViewerAnalyticsTests");
        QCoreApplication::setApplicationName(QUuid::createUuid().toString(QUuid::WithoutBraces));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        ASSERT_TRUE(server.listen(QHostAddress::LocalHost));
        host = QUrl(QString("http://127.0.0.1:%1").arg(server.serverPort()));
    }

    void TearDown() override
    {
        QSettings::setDefaultFormat(oldFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, oldSettingsPath);
        QCoreApplication::setOrganizationName(oldOrganization);
        QCoreApplication::setApplicationName(oldApplication);
    }

    QByteArray ReceiveRequest()
    {
        QElapsedTimer timer;
        timer.start();
        QTcpSocket * socket = nullptr;
        QByteArray request;
        while (timer.elapsed() < 3000)
        {
            QCoreApplication::processEvents();
            if (!socket && server.hasPendingConnections())
                socket = server.nextPendingConnection();
            if (!socket)
                continue;
            request += socket->readAll();
            const auto split = request.indexOf("\r\n\r\n");
            if (split < 0)
                continue;
            int length = -1;
            for (const auto & line : request.left(split).split('\n'))
                if (line.toLower().startsWith("content-length:"))
                    length = line.mid(line.indexOf(':') + 1).trimmed().toInt();
            if (length < 0 || request.size() < split + 4 + length)
                continue;
            socket->write("HTTP/1.1 200 OK\r\nContent-Length: 2\r\nConnection: close\r\n\r\n{}");
            socket->disconnectFromHost();
            return request;
        }
        return {};
    }

    int argc = 1;
    char name[14] = "AnalyticsTest";
    char * argv[2] = { name, nullptr };
    std::unique_ptr<QCoreApplication> app;
    QTemporaryDir directory;
    QTcpServer server;
    QUrl host;
    QString oldOrganization;
    QString oldApplication;
    QString oldSettingsPath;
    QSettings::Format oldFormat;
};

TEST_F(AnalyticsTest, EmptyTokenDoesNotStoreIdentityOrSendRequest)
{
    PastViewer::Analytics analytics({}, host);
    analytics.Track("app_opened");
    QCoreApplication::processEvents();
    EXPECT_FALSE(QSettings().contains("analytics/installationId"));
    EXPECT_FALSE(server.waitForNewConnection(50));
}

TEST_F(AnalyticsTest, SendsAnonymousEventAndReusesPersistedIdentity)
{
    QString installationId;
    for (int launch = 0; launch < 2; ++launch)
    {
        PastViewer::Analytics analytics("test-project-token", host);
        analytics.Track("app_opened", {{"$os", "incorrect"}, {"$os_version", "incorrect"}});
        const auto request = ReceiveRequest();
        ASSERT_FALSE(request.isEmpty());
        EXPECT_TRUE(request.startsWith("POST /i/v0/e/ HTTP/1.1\r\n"));
        EXPECT_TRUE(request.toLower().contains("content-type: application/json"));
        const auto payload = QJsonDocument::fromJson(request.mid(request.indexOf("\r\n\r\n") + 4)).object();
        EXPECT_EQ(payload.value("api_key").toString(), "test-project-token");
        EXPECT_EQ(payload.value("event").toString(), "app_opened");
        const auto properties = payload.value("properties").toObject();
        EXPECT_EQ(properties.value("$process_person_profile"), QJsonValue(false));
        EXPECT_EQ(properties.value("$os").toString(), QSysInfo::productType());
        EXPECT_EQ(properties.value("$os_version").toString(), QSysInfo::productVersion());
#if defined(Q_OS_DARWIN) || defined(Q_OS_ANDROID)
        EXPECT_FALSE(properties.value("$device_model").toString().isEmpty());
#endif
        const auto id = properties.value("distinct_id").toString();
        EXPECT_FALSE(QUuid(id).isNull());
        EXPECT_EQ(id, QSettings().value("analytics/installationId").toString());
        if (launch == 0)
            installationId = id;
        else
            EXPECT_EQ(id, installationId);
    }
}
} // namespace
