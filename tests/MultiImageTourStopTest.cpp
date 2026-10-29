#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QList>
#include <QMetaObject>
#include <QMetaProperty>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariant>

#include <gtest/gtest.h>

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Controllers/TourController/TourController.h"
#include "App/Models/TourDraftsModel/TourDraftsModel.h"
#include "App/Tours/DisplayTours.h"
#include "App/Tours/Tours.h"

#define private public
#include "App/Models/AvailableToursModel/AvailableToursModel.h"
#undef private

// Same layout as AvailableToursModel::Impl. The test disconnects the live
// request the constructor starts, then feeds a tours payload directly.
struct AvailableToursModel::Impl
{
    QNetworkAccessManager networkManager {};
    std::vector<DisplayTour> tours;
};

namespace {

constexpr auto kAbcSha256 = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
const QByteArray kAbcBytes = QByteArrayLiteral("abc");
const QByteArray kSecondBytes = QByteArrayLiteral("second-image-bytes");
const QByteArray kNarrationBytes = QByteArrayLiteral("narration-take");

void EnsureCoreApplication()
{
    if (QCoreApplication::instance())
        return;

    static int argc = 1;
    static char executableName[] = "PastViewerTests";
    static char * argv[] = { executableName, nullptr };
    static auto * const application = new QCoreApplication(argc, argv);
}

QString ContentHash(const QByteArray & bytes)
{
    return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

class JsonBodyReply : public QNetworkReply
{
public:
    explicit JsonBodyReply(const QByteArray & body, QObject * parent = nullptr)
        : QNetworkReply(parent)
        , m_body(body)
    {
        setError(NoError, QString());
        setFinished(true);
        open(QIODevice::ReadOnly);
    }

    void abort() override
    {
    }

    qint64 bytesAvailable() const override
    {
        return static_cast<qint64>(m_body.size()) - m_offset + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char * data, qint64 maxSize) override
    {
        if (maxSize <= 0)
            return 0;

        const auto remaining = static_cast<qint64>(m_body.size()) - m_offset;
        if (remaining <= 0)
            return -1;

        const auto count = qMin(maxSize, remaining);
        std::memcpy(data, m_body.constData() + static_cast<int>(m_offset), static_cast<std::size_t>(count));
        m_offset += count;
        return count;
    }

private:
    QByteArray m_body {};
    qint64 m_offset = 0;
};

template <typename Stop>
void AssignStopImages(Stop & stop, const QList<QUrl> & images)
{
    if constexpr (requires { stop.localImagePaths = images; })
        stop.localImagePaths = images;
    else
        ADD_FAILURE() << "TourStop must store localImagePaths (list of QUrl)";
}

template <typename Stop>
QList<QUrl> StopLocalImagePaths(const Stop & stop)
{
    if constexpr (requires { stop.localImagePaths; })
        return stop.localImagePaths;
    else
        return {};
}

template <typename Stop>
QStringList StopImageFileHashes(const Stop & stop)
{
    if constexpr (requires { stop.imageFileHashes; })
        return stop.imageFileHashes;
    else
        return {};
}

template <typename ModelOrController>
void CreateTourStopWithImages(
    ModelOrController & target,
    const QString & title,
    const QString & description,
    const QList<QUrl> & images,
    const QUrl & audioFile,
    const QGeoCoordinate & coordinate = QGeoCoordinate())
{
    if constexpr (requires { target.CreateTourStop(title, description, images, audioFile, coordinate); })
        target.CreateTourStop(title, description, images, audioFile, coordinate);
    else
        ADD_FAILURE() << "CreateTourStop must accept a list of image files";
}

template <typename ModelOrController>
QString UpdateDraftStopWithImages(
    ModelOrController & target,
    qlonglong id,
    const QString & title,
    const QString & description,
    const QList<QUrl> & images,
    const QUrl & audioFile)
{
    if constexpr (requires { target.UpdateDraftStop(id, title, description, images, audioFile); })
    {
        return target.UpdateDraftStop(id, title, description, images, audioFile);
    }
    else
    {
        ADD_FAILURE() << "UpdateDraftStop must accept a list of image files";
        return QStringLiteral("UpdateDraftStop list API missing");
    }
}

QStringList DisplayStopImageUrls(const DisplayTourStop & stop)
{
    const QMetaObject & metaObject = DisplayTourStop::staticMetaObject;
    const int propertyIndex = metaObject.indexOfProperty("imageUrls");
    if (propertyIndex < 0)
    {
        ADD_FAILURE() << "DisplayTourStop must expose imageUrls";
        return {};
    }

    const QVariant value = metaObject.property(propertyIndex).readOnGadget(&stop);
    if (value.canConvert<QStringList>())
        return value.value<QStringList>();
    if (value.canConvert<QList<QString>>())
        return QStringList(value.value<QList<QString>>());
    if (value.canConvert<QVariantList>())
    {
        QStringList urls;
        for (const auto & item : value.toList())
            urls.append(item.toString());
        return urls;
    }

    ADD_FAILURE() << "DisplayTourStop.imageUrls must be a list of strings";
    return {};
}

class MultiImageTourStop : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        EnsureCoreApplication();
        QCoreApplication::setOrganizationName(QStringLiteral("PastViewerMultiImageTourStopTests"));
        QCoreApplication::setApplicationName(QStringLiteral("PastViewerMultiImageTourStopTests"));
        QStandardPaths::setTestModeEnabled(true);
    }

    void SetUp() override
    {
        QDir(Tours::GetDraftsLocation()).removeRecursively();
        ASSERT_TRUE(QDir().mkpath(Tours::GetDraftsLocation()));
        media = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(media->isValid());
        position = std::make_unique<PositionSourceAdapter>(nullptr);
        model = std::make_unique<TourDraftsModel>(*position);
        controller = std::make_unique<TourController>(*position);
    }

    void TearDown() override
    {
        controller.reset();
        model.reset();
        position.reset();
        media.reset();
        QDir(Tours::GetDraftsLocation()).removeRecursively();
    }

    QUrl WriteMedia(const QString & relativeName, const QByteArray & bytes) const
    {
        const auto path = media->filePath(relativeName);
        EXPECT_TRUE(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) << path.toStdString();
        EXPECT_EQ(file.write(bytes), static_cast<qint64>(bytes.size())) << path.toStdString();
        file.close();
        return QUrl::fromLocalFile(path);
    }

    QJsonObject ReadDraftObject(const QString & title) const
    {
        QFile file(Tours::GetDraftFileLocation(title));
        EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << Tours::GetDraftFileLocation(title).toStdString();
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(file.readAll(), &error);
        EXPECT_EQ(error.error, QJsonParseError::NoError) << error.errorString().toStdString();
        EXPECT_TRUE(document.isObject());
        return document.object();
    }

    QJsonObject FindStop(const QJsonObject & tour, const QString & title) const
    {
        for (const auto & value : tour.value(QStringLiteral("stops")).toArray())
        {
            const auto stop = value.toObject();
            if (stop.value(QStringLiteral("title")).toString() == title)
                return stop;
        }
        ADD_FAILURE() << "missing stop " << title.toStdString();
        return {};
    }

    std::unique_ptr<QTemporaryDir> media;
    std::unique_ptr<PositionSourceAdapter> position;
    std::unique_ptr<TourDraftsModel> model;
    std::unique_ptr<TourController> controller;
};

}

TEST_F(MultiImageTourStop, CreateTourStopAcceptsImageListAndRoundTripsDraft)
{
    const auto coverUrl = WriteMedia(QStringLiteral("cover.png"), kAbcBytes);
    const auto firstImage = WriteMedia(QStringLiteral("stop-a.jpg"), kAbcBytes);
    const auto secondImage = WriteMedia(QStringLiteral("stop-b.png"), kSecondBytes);
    const auto audioUrl = WriteMedia(QStringLiteral("stop.mp3"), kNarrationBytes);
    const auto secondHash = ContentHash(kSecondBytes);

    model->CreateNewTour(QStringLiteral("MultiDraft"), QStringLiteral("walk"), coverUrl, QStringLiteral("en"));
    CreateTourStopWithImages(
        *model,
        QStringLiteral("Gate"),
        QStringLiteral("Start here"),
        { firstImage, secondImage },
        audioUrl,
        QGeoCoordinate(55.75, 37.61));

    const auto drafts = model->GetTourDrafts();
    ASSERT_EQ(drafts.size(), std::size_t { 1 });
    ASSERT_EQ(drafts.front().stops.size(), 1);
    EXPECT_EQ(StopLocalImagePaths(drafts.front().stops.front()), (QList<QUrl> { firstImage, secondImage }));
    EXPECT_EQ(
        StopImageFileHashes(drafts.front().stops.front()),
        (QStringList { QString::fromLatin1(kAbcSha256), secondHash }));

    const auto draft = ReadDraftObject(QStringLiteral("MultiDraft"));
    const auto stop = FindStop(draft, QStringLiteral("Gate"));
    ASSERT_TRUE(stop.value(QStringLiteral("localImagePaths")).isArray());
    ASSERT_EQ(stop.value(QStringLiteral("localImagePaths")).toArray().size(), 2);
    EXPECT_EQ(stop.value(QStringLiteral("localImagePaths")).toArray().at(0).toString(), firstImage.toString());
    EXPECT_EQ(stop.value(QStringLiteral("localImagePaths")).toArray().at(1).toString(), secondImage.toString());
    ASSERT_TRUE(stop.value(QStringLiteral("imageFileHashes")).isArray());
    ASSERT_EQ(stop.value(QStringLiteral("imageFileHashes")).toArray().size(), 2);
    EXPECT_EQ(stop.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString(), QString::fromLatin1(kAbcSha256));
    EXPECT_EQ(stop.value(QStringLiteral("imageFileHashes")).toArray().at(1).toString(), secondHash);

    PositionSourceAdapter reloadPosition { nullptr };
    TourDraftsModel reloaded { reloadPosition };
    ASSERT_EQ(reloaded.rowCount(), 1);
    const auto loaded = reloaded.GetTourDrafts().at(0);
    ASSERT_EQ(loaded.stops.size(), 1);
    EXPECT_EQ(StopLocalImagePaths(loaded.stops.at(0)), (QList<QUrl> { firstImage, secondImage }));
    EXPECT_EQ(
        StopImageFileHashes(loaded.stops.at(0)),
        (QStringList { QString::fromLatin1(kAbcSha256), secondHash }));
}

TEST_F(MultiImageTourStop, UpdateDraftStopAcceptsImageList)
{
    const auto coverUrl = WriteMedia(QStringLiteral("cover-update.png"), kAbcBytes);
    const auto firstImage = WriteMedia(QStringLiteral("one.jpg"), kAbcBytes);
    const auto secondImage = WriteMedia(QStringLiteral("two.png"), kSecondBytes);
    const auto replacementA = WriteMedia(QStringLiteral("three.jpg"), QByteArrayLiteral("three"));
    const auto replacementB = WriteMedia(QStringLiteral("four.png"), QByteArrayLiteral("four"));
    const auto audioUrl = WriteMedia(QStringLiteral("voice.mp3"), kNarrationBytes);

    model->CreateNewTour(QStringLiteral("UpdateImages"), QStringLiteral("walk"), coverUrl, {});
    CreateTourStopWithImages(
        *model,
        QStringLiteral("Gate"),
        QStringLiteral("old"),
        { firstImage },
        audioUrl);
    ASSERT_EQ(model->GetTourDrafts().size(), std::size_t { 1 });
    ASSERT_EQ(model->GetTourDrafts().front().stops.size(), 1);
    const auto stopId = model->GetTourDrafts().front().stops.front().id;

    ASSERT_TRUE(model->SelectDraft(model->GetTourDrafts().front().id));
    const auto error = UpdateDraftStopWithImages(
        *model,
        stopId,
        QStringLiteral("Gate"),
        QStringLiteral("updated"),
        { replacementA, replacementB },
        audioUrl);
    EXPECT_TRUE(error.isEmpty()) << error.toStdString();

    const auto updated = model->GetTourDrafts().front().stops.front();
    EXPECT_EQ(updated.description, QStringLiteral("updated"));
    EXPECT_EQ(StopLocalImagePaths(updated), (QList<QUrl> { replacementA, replacementB }));
    EXPECT_EQ(
        StopImageFileHashes(updated),
        (QStringList { ContentHash(QByteArrayLiteral("three")), ContentHash(QByteArrayLiteral("four")) }));
}

TEST_F(MultiImageTourStop, TourToJsonSerializesTwoImageHashesPerStop)
{
    const auto coverUrl = WriteMedia(QStringLiteral("json-cover.png"), kAbcBytes);
    const auto firstImage = WriteMedia(QStringLiteral("json-a.jpg"), kAbcBytes);
    const auto secondImage = WriteMedia(QStringLiteral("json-b.png"), kSecondBytes);
    const auto audioUrl = WriteMedia(QStringLiteral("json.mp3"), kNarrationBytes);
    const auto secondHash = ContentHash(kSecondBytes);

    Tours::TourStop stop;
    stop.id = 1;
    stop.title = QStringLiteral("Gate");
    stop.description = QStringLiteral("Start here");
    AssignStopImages(stop, { firstImage, secondImage });
    stop.localAudioPath = audioUrl;
    stop.coords = QGeoCoordinate(55.75, 37.61);

    Tours::Tour tour;
    tour.title = QStringLiteral("TwoImages");
    tour.description = QStringLiteral("walk");
    tour.localImagePath = coverUrl;
    tour.lang = QStringLiteral("en");
    tour.stops = { stop };

    const auto json = Tours::TourToJson(tour);
    ASSERT_FALSE(json.isEmpty());
    ASSERT_EQ(json.value(QStringLiteral("stops")).toArray().size(), 1);
    const auto stopJson = json.value(QStringLiteral("stops")).toArray().at(0).toObject();
    ASSERT_TRUE(stopJson.value(QStringLiteral("imageFileHashes")).isArray());
    ASSERT_EQ(stopJson.value(QStringLiteral("imageFileHashes")).toArray().size(), 2);
    EXPECT_EQ(
        stopJson.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString(),
        QString::fromLatin1(kAbcSha256));
    EXPECT_EQ(stopJson.value(QStringLiteral("imageFileHashes")).toArray().at(1).toString(), secondHash);
    EXPECT_FALSE(stopJson.contains(QStringLiteral("imageFileHash")));
    EXPECT_FALSE(stopJson.contains(QStringLiteral("imageFile")));
    EXPECT_FALSE(stopJson.contains(QStringLiteral("imageFiles")));
}

TEST_F(MultiImageTourStop, ControllerCreateTourStopForwardsImageList)
{
    const auto coverUrl = WriteMedia(QStringLiteral("controller-cover.png"), kAbcBytes);
    const auto firstImage = WriteMedia(QStringLiteral("controller-a.jpg"), kAbcBytes);
    const auto secondImage = WriteMedia(QStringLiteral("controller-b.png"), kSecondBytes);

    controller->CreateNewTour(QStringLiteral("ControllerMulti"), QStringLiteral("walk"), coverUrl, {});
    CreateTourStopWithImages(
        *controller,
        QStringLiteral("Gate"),
        QStringLiteral("stop"),
        { firstImage, secondImage },
        QUrl());

    auto * draftsModel = qobject_cast<TourDraftsModel *>(controller->GetDraftsModel());
    ASSERT_NE(draftsModel, nullptr);
    const auto drafts = draftsModel->GetTourDrafts();
    ASSERT_EQ(drafts.size(), std::size_t { 1 });
    ASSERT_EQ(drafts.front().stops.size(), 1);
    EXPECT_EQ(StopLocalImagePaths(drafts.front().stops.front()), (QList<QUrl> { firstImage, secondImage }));
}

TEST_F(MultiImageTourStop, AvailableToursModelMapsStopImageUrlsList)
{
    const QString firstUrl = QStringLiteral("https://pastviewer.com/media/stops/gate-1.jpg");
    const QString secondUrl = QStringLiteral("https://pastviewer.com/media/stops/gate-2.jpg");

    const QJsonObject stop {
        { QStringLiteral("title"),       QStringLiteral("Gate")                                        },
        { QStringLiteral("description"), QStringLiteral("Start here")                                  },
        { QStringLiteral("imageUrls"),   QJsonArray { firstUrl, secondUrl }                            },
        { QStringLiteral("audioUrl"),    QStringLiteral("https://pastviewer.com/media/stops/gate.mp3") },
        { QStringLiteral("coords"),
         QJsonObject {
                { QStringLiteral("latitude"), 55.75 },
                { QStringLiteral("longitude"), 37.61 },
            }                                                                                          },
    };
    const QJsonObject emptyImagesStop {
        { QStringLiteral("title"),       QStringLiteral("Pier")     },
        { QStringLiteral("description"), QStringLiteral("No media") },
        { QStringLiteral("imageUrls"),   QJsonArray {}              },
    };
    const QJsonObject tour {
        { QStringLiteral("title"),       QStringLiteral("Belgrade walk")                                },
        { QStringLiteral("description"), QStringLiteral("A short walking tour.")                        },
        { QStringLiteral("imageUrl"),    QStringLiteral("https://pastviewer.com/media/tours/cover.jpg") },
        { QStringLiteral("distance"),    1250                                                           },
        { QStringLiteral("stopCount"),   2                                                              },
        { QStringLiteral("path"),        QJsonObject { { QStringLiteral("coords"), QJsonArray {} } }    },
        { QStringLiteral("stops"),       QJsonArray { stop, emptyImagesStop }                           },
    };
    const auto payload = QJsonDocument(QJsonObject {
                                           { QStringLiteral("items"), QJsonArray { tour } }
    })
                             .toJson(QJsonDocument::Compact);

    AvailableToursModel available;
    QObject::disconnect(&available.m_impl->networkManager, nullptr, &available, nullptr);
    auto * reply = new JsonBodyReply(payload);
    available.OnNetworkReplyFinished(reply);
    ASSERT_EQ(available.rowCount(), 1);

    const auto value = available.data(available.index(0, 0), AvailableToursModel::Stops);
    ASSERT_TRUE(value.canConvert<QList<DisplayTourStop>>());
    const auto stops = value.value<QList<DisplayTourStop>>();
    ASSERT_EQ(stops.size(), 2);

    EXPECT_EQ(DisplayStopImageUrls(stops.at(0)), (QStringList { firstUrl, secondUrl }));
    EXPECT_TRUE(DisplayStopImageUrls(stops.at(1)).isEmpty());
}
