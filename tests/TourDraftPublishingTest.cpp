#include <algorithm>
#include <cstring>
#include <memory>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "App/Models/TourDraftsModel/TourDraftsModel.h"

namespace {
class ControlledReply : public QNetworkReply
{
public:
    ControlledReply(const QNetworkRequest & request, QNetworkAccessManager::Operation operation, QObject * parent)
        : QNetworkReply(parent)
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(operation);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }

    void Complete(int status, const QByteArray & body, const QString & transportError = {})
    {
        m_body = body;
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, status);
        if (!transportError.isEmpty())
            setError(QNetworkReply::UnknownNetworkError, transportError);
        else if (status >= 400)
            setError(QNetworkReply::ContentOperationNotPermittedError, "Request rejected");
        setFinished(true);
        emit readyRead();
        emit finished();
    }

    void abort() override { Complete(0, {}, "Cancelled"); }
    qint64 bytesAvailable() const override { return m_body.size() - m_position + QNetworkReply::bytesAvailable(); }

protected:
    qint64 readData(char * data, qint64 size) override
    {
        const auto count = std::min(size, static_cast<qint64>(m_body.size() - m_position));
        if (count <= 0)
            return -1;
        std::memcpy(data, m_body.constData() + m_position, count);
        m_position += count;
        return count;
    }

private:
    QByteArray m_body;
    qint64 m_position { 0 };
};

class ControlledNetwork : public QNetworkAccessManager
{
public:
    struct Request
    {
        QString path;
        QByteArray body;
        QPointer<ControlledReply> reply;
    };
    QList<Request> requests;

    void AssetSucceeded(int index, const QString & idOverride = {})
    {
        const auto body = requests.at(index).body;
        const auto match = QRegularExpression("name=\"(?:image|audio)FileHash\"\\r\\n\\r\\n([a-f0-9]{64})")
                               .match(QString::fromLatin1(body));
        ASSERT_TRUE(match.hasMatch()) << body.toStdString();
        const auto kind = body.contains("name=\"audioFileHash\"") ? "audio" : "image";
        const auto response = QJsonDocument(QJsonObject {
            { "id", idOverride.isEmpty() ? match.captured(1) : idOverride },
            { "kind", kind },
        }).toJson();
        requests.at(index).reply->Complete(201, response);
    }

    void TourSucceeded(int index)
    {
        requests.at(index).reply->Complete(201, R"({"meta":{"id":"tour_0123456789abcdef"}})");
    }

protected:
    QNetworkReply * createRequest(Operation operation, const QNetworkRequest & request, QIODevice * data) override
    {
        auto * reply = new ControlledReply(request, operation, this);
        requests.append({ request.url().path(), data ? data->readAll() : QByteArray(), reply });
        return reply;
    }
};

class TourDraftPublishing : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QCoreApplication::instance())
        {
            static int argc = 1;
            static char name[] = "PastViewerTests";
            static char * argv[] = { name, nullptr };
            static auto * app = new QCoreApplication(argc, argv);
        }
        QCoreApplication::setOrganizationName("PastViewerPublishingTests");
        QCoreApplication::setApplicationName("PastViewerPublishingTests");
        QStandardPaths::setTestModeEnabled(true);
    }

    void SetUp() override
    {
        QDir(Tours::GetDraftsLocation()).removeRecursively();
        ASSERT_TRUE(QDir().mkpath(Tours::GetDraftsLocation()));
        ASSERT_TRUE(media.isValid());
        position = std::make_unique<PositionSourceAdapter>(nullptr);
        model = std::make_unique<TourDraftsModel>(*position, nullptr, &network);
        QObject::connect(model.get(), &TourDraftsModel::ImageUploadFailed, &observer,
            [this](const QString & message) { error = message; });
    }

    void TearDown() override
    {
        model.reset();
        position.reset();
        QDir(Tours::GetDraftsLocation()).removeRecursively();
    }

    QUrl File(const QString & name, const QByteArray & bytes)
    {
        const auto path = media.filePath(name);
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        EXPECT_EQ(file.write(bytes), bytes.size());
        return QUrl::fromLocalFile(path);
    }

    QUrl Image(const QString & name)
    {
        // MIME detection uses the PNG signature; distinct bytes give distinct hashes.
        return File(name, QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aM2kAAAAASUVORK5CYII=") + name.toUtf8());
    }

    void CreateWithMedia()
    {
        const auto cover = Image("cover.png");
        const auto image = Image("stop.png");
        const auto audio = File("audio.wav", QByteArray::fromHex("524946462600000057415645666d74201000000001000100401f0000803e00000200100064617461020000000000"));
        ASSERT_TRUE(model->CreateNewTour("Queued tour", "", cover, "en").isEmpty());
        // Repeated URLs are uploaded once, while both references remain in JSON.
        model->CreateTourStop("Stop", "", { cover, image }, audio, QGeoCoordinate(44.8, 20.4));
    }

    bool Publish(int row = 0) { return model->setData(model->index(row), true, TourDraftsModel::Publish); }
    bool Busy(int row = 0) { return model->data(model->index(row), TourDraftsModel::Uploading).toBool(); }

    ControlledNetwork network;
    QTemporaryDir media;
    QObject observer;
    QString error;
    std::unique_ptr<PositionSourceAdapter> position;
    std::unique_ptr<TourDraftsModel> model;
};

TEST_F(TourDraftPublishing, WaitsForEveryAssetAndTourConfirmationWhileRejectingRepeatedPublish)
{
    CreateWithMedia();
    ASSERT_TRUE(Publish());
    EXPECT_TRUE(Busy());
    EXPECT_FALSE(Publish());
    ASSERT_EQ(network.requests.size(), 1);
    EXPECT_EQ(network.requests.at(0).path, "/api/v1/admin/assets");
    EXPECT_FALSE(model->setData(model->index(0), true, TourDraftsModel::Delete));
    EXPECT_FALSE(model->UpdateDraftDetails(model->GetTourDrafts().front().id, "Renamed", "", {}).isEmpty());

    for (int i = 0; i < 3; ++i)
    {
        ASSERT_EQ(network.requests.size(), i + 1);
        EXPECT_TRUE(Busy());
        network.AssetSucceeded(i);
    }
    ASSERT_EQ(network.requests.size(), 4);
    EXPECT_EQ(network.requests.at(3).path, "/api/v1/admin/tours");
    const auto payload = QJsonDocument::fromJson(network.requests.at(3).body).object();
    EXPECT_EQ(payload.value("title").toString(), "Queued tour");
    EXPECT_TRUE(Busy());
    EXPECT_TRUE(QFile::exists(Tours::GetDraftFileLocation("Queued tour")));
    network.TourSucceeded(3);
    EXPECT_EQ(model->rowCount(), 0);
    EXPECT_FALSE(QFile::exists(Tours::GetDraftFileLocation("Queued tour")));
    EXPECT_TRUE(error.isEmpty());
}

TEST_F(TourDraftPublishing, AssetFailureReportsFileAndBackendReasonWithoutSubmittingTour)
{
    CreateWithMedia();
    ASSERT_TRUE(Publish());
    network.AssetSucceeded(0);
    ASSERT_EQ(network.requests.size(), 2);
    network.requests.at(1).reply->Complete(400, R"({"error":{"code":"INVALID_MEDIA_TYPE","message":"Unsupported media type","details":["image/heic"]}})");
    EXPECT_FALSE(Busy());
    EXPECT_EQ(network.requests.size(), 2);
    EXPECT_EQ(model->rowCount(), 1);
    EXPECT_TRUE(QFile::exists(Tours::GetDraftFileLocation("Queued tour")));
    EXPECT_TRUE(error.contains("stop.png"));
    EXPECT_TRUE(error.contains("INVALID_MEDIA_TYPE"));
    EXPECT_TRUE(error.contains("image/heic"));
    EXPECT_TRUE(Publish());
    EXPECT_TRUE(Busy());
    EXPECT_EQ(network.requests.size(), 3);
}

TEST_F(TourDraftPublishing, TransportFailureAndInvalidAssetHashBothClearBusyAndPreserveDraft)
{
    CreateWithMedia();
    ASSERT_TRUE(Publish());
    network.requests.at(0).reply->Complete(0, {}, "GOAWAY received, cannot start a request");
    EXPECT_FALSE(Busy());
    EXPECT_TRUE(error.contains("GOAWAY"));
    EXPECT_EQ(network.requests.size(), 1);
    ASSERT_TRUE(Publish());
    network.AssetSucceeded(1, QString(64, '0'));
    EXPECT_FALSE(Busy());
    EXPECT_TRUE(error.contains("invalid upload response"));
    EXPECT_EQ(network.requests.size(), 2);
    EXPECT_EQ(model->rowCount(), 1);
    EXPECT_TRUE(QFile::exists(Tours::GetDraftFileLocation("Queued tour")));
}

TEST_F(TourDraftPublishing, MediaFreeTourSubmitsImmediatelyAndServerErrorsKeepDraft)
{
    ASSERT_TRUE(model->CreateNewTour("No media", "", {}, "en").isEmpty());
    ASSERT_TRUE(Publish());
    ASSERT_EQ(network.requests.size(), 1);
    EXPECT_EQ(network.requests.at(0).path, "/api/v1/admin/tours");
    EXPECT_TRUE(Busy());
    network.requests.at(0).reply->Complete(400, R"({"error":{"code":"VALIDATION_ERROR","message":"Tour validation failed","details":["lang is required"]}})");
    EXPECT_FALSE(Busy());
    EXPECT_TRUE(error.contains("lang is required"));
    EXPECT_TRUE(QFile::exists(Tours::GetDraftFileLocation("No media")));
    ASSERT_TRUE(Publish());
    network.requests.at(1).reply->Complete(201, R"({"meta":{}})");
    EXPECT_FALSE(Busy());
    EXPECT_EQ(model->rowCount(), 1);
    EXPECT_TRUE(error.contains("invalid tour upload response"));
}

TEST_F(TourDraftPublishing, ConcurrentOperationsAndStandaloneUploadsHaveIndependentCompletion)
{
    const auto image = Image("one.png");
    ASSERT_TRUE(model->CreateNewTour("First", "", image, "en").isEmpty());
    ASSERT_TRUE(model->CreateNewTour("Second", "", image, "en").isEmpty());
    ASSERT_TRUE(Publish(0));
    ASSERT_TRUE(Publish(1));
    int standaloneCompletions = 0;
    QObject::connect(model.get(), &TourDraftsModel::AssetsUploadFinished, &observer,
        [&] { ++standaloneCompletions; });
    model->UploadAssets({ image });
    ASSERT_EQ(network.requests.size(), 3);
    network.AssetSucceeded(2);
    EXPECT_EQ(standaloneCompletions, 1);
    EXPECT_TRUE(Busy(0));
    EXPECT_TRUE(Busy(1));
    network.AssetSucceeded(0);
    ASSERT_EQ(network.requests.size(), 4);
    network.TourSucceeded(3); // Remove row 0; the other uploading draft moves from row 1 to row 0.
    ASSERT_EQ(model->rowCount(), 1);
    EXPECT_TRUE(Busy(0));
    network.AssetSucceeded(1);
    ASSERT_EQ(network.requests.size(), 5);
    network.TourSucceeded(4);
    EXPECT_EQ(model->rowCount(), 0);
    EXPECT_EQ(standaloneCompletions, 1);
}

TEST_F(TourDraftPublishing, MediaChangedAfterPreflightStopsBeforeSubmittingTour)
{
    CreateWithMedia();
    ASSERT_TRUE(Publish());
    File("stop.png", QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aM2kAAAAASUVORK5CYII=") + "changed");
    network.AssetSucceeded(0);
    EXPECT_EQ(network.requests.size(), 1);
    EXPECT_FALSE(Busy());
    EXPECT_TRUE(error.contains("file changed"));
    EXPECT_TRUE(QFile::exists(Tours::GetDraftFileLocation("Queued tour")));
}
}
