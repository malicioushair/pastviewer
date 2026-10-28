#include <memory>
#include <ostream>
#include <utility>

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
#include <QJsonValue>
#include <QList>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUrl>

#include <gtest/gtest.h>

void PrintTo(const QString & value, std::ostream * stream)
{
	*stream << '"' << value.toStdString() << '"';
}

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Controllers/TourController/TourController.h"
#include "App/Models/TourDraftsModel/TourDraftsModel.h"
#include "App/Tours/Tours.h"

namespace {

constexpr auto kAbcSha256 = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
constexpr auto kEmptySha256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

const QByteArray kAbcBytes = QByteArrayLiteral("abc");
const QByteArray kNarrationBytes = QByteArrayLiteral("narration-take");

QByteArray CoverBytes()
{
	QByteArray bytes("cover-bytes");
	bytes.append('\0');
	bytes.append('\xff');
	return bytes;
}

QString ContentHash(const QByteArray & bytes)
{
	return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

void EnsureCoreApplication()
{
	if (QCoreApplication::instance())
		return;

	// Leak the application for process lifetime. TourController's
	// QNetworkAccessManager leaves QObjects that crash if it is destroyed first.
	static int argc = 1;
	static char executableName[] = "PastViewerTests";
	static char * argv[] = { executableName, nullptr };
	static auto * const application = new QCoreApplication(argc, argv);
}

struct ControllerSession
{
	PositionSourceAdapter position { nullptr };
	TourController controller { position };
};

class FileHash : public ::testing::Test
{
protected:
	static void SetUpTestSuite()
	{
		EnsureCoreApplication();
		QCoreApplication::setOrganizationName(QStringLiteral("PastViewerFileHashTests"));
		QCoreApplication::setApplicationName(QStringLiteral("PastViewerFileHashTests"));
		QStandardPaths::setTestModeEnabled(true);
	}

	void SetUp() override
	{
		QDir(Tours::GetDraftsLocation()).removeRecursively();
		ASSERT_TRUE(QDir().mkpath(Tours::GetDraftsLocation()));
		media = std::make_unique<QTemporaryDir>();
		ASSERT_TRUE(media->isValid());
	}

	void TearDown() override
	{
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

	QString DraftPath(const QString & title) const
	{
		return Tours::GetDraftFileLocation(title);
	}

	QByteArray ReadDraftBytes(const QString & title) const
	{
		QFile file(DraftPath(title));
		EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << DraftPath(title).toStdString();
		return file.readAll();
	}

	QJsonObject ReadDraftObject(const QString & title) const
	{
		QJsonParseError error;
		const auto document = QJsonDocument::fromJson(ReadDraftBytes(title), &error);
		EXPECT_EQ(error.error, QJsonParseError::NoError) << error.errorString().toStdString();
		EXPECT_TRUE(document.isObject());
		return document.object();
	}

	void WriteDraftObject(const QString & title, const QJsonObject & object) const
	{
		QFile file(DraftPath(title));
		ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate)) << DraftPath(title).toStdString();
		const auto bytes = QJsonDocument(object).toJson(QJsonDocument::Compact);
		ASSERT_EQ(file.write(bytes), static_cast<qint64>(bytes.size()));
	}

	std::unique_ptr<QTemporaryDir> media;
};

void ExpectContentHash(const QJsonObject & object, const QString & key, const QString & expected)
{
	EXPECT_TRUE(object.contains(key)) << key.toStdString();
	const auto actual = object.value(key).toString();
	EXPECT_TRUE(object.value(key).isString()) << key.toStdString();
	EXPECT_EQ(actual, expected) << key.toStdString();
	EXPECT_TRUE(QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(actual).hasMatch()) << actual.toStdString();
	EXPECT_NE(object.value(QStringLiteral("imageAssetId")).toString(), expected) << key.toStdString();
}

void ExpectEmptyHash(const QJsonObject & object, const QString & key)
{
	EXPECT_TRUE(object.contains(key)) << key.toStdString();
	EXPECT_TRUE(object.value(key).isString()) << key.toStdString();
	EXPECT_EQ(object.value(key).toString(), QString());
}

void ExpectPublishRejected(const Tours::Tour & tour)
{
	EXPECT_TRUE(Tours::TourToJson(tour).isEmpty());
}

void ExpectNoForbiddenPublishKeys(const QJsonObject & object)
{
	EXPECT_FALSE(object.contains(QStringLiteral("imageFile")));
	EXPECT_FALSE(object.contains(QStringLiteral("imageFiles")));
	EXPECT_FALSE(object.contains(QStringLiteral("audioFile")));
	EXPECT_FALSE(object.contains(QStringLiteral("id")));
	EXPECT_FALSE(object.contains(QStringLiteral("localImagePath")));
	EXPECT_FALSE(object.contains(QStringLiteral("localImagePaths")));
	EXPECT_FALSE(object.contains(QStringLiteral("localAudioPath")));
}

void ExpectCoords(const QJsonObject & object, double latitude, double longitude)
{
	ASSERT_TRUE(object.contains(QStringLiteral("coords")));
	ASSERT_TRUE(object.value(QStringLiteral("coords")).isObject());
	const auto coords = object.value(QStringLiteral("coords")).toObject();
	EXPECT_DOUBLE_EQ(coords.value(QStringLiteral("latitude")).toDouble(), latitude);
	EXPECT_DOUBLE_EQ(coords.value(QStringLiteral("longitude")).toDouble(), longitude);
}

void ExpectImageFileHashes(const QJsonObject & stop, const QStringList & expected)
{
	ASSERT_TRUE(stop.contains(QStringLiteral("imageFileHashes")));
	ASSERT_TRUE(stop.value(QStringLiteral("imageFileHashes")).isArray());
	const auto hashes = stop.value(QStringLiteral("imageFileHashes")).toArray();
	ASSERT_EQ(hashes.size(), expected.size());
	for (qsizetype i = 0; i < expected.size(); ++i)
	{
		EXPECT_TRUE(hashes.at(i).isString()) << i;
		EXPECT_EQ(hashes.at(i).toString(), expected.at(i)) << i;
		if (!expected.at(i).isEmpty())
		{
			EXPECT_TRUE(QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(expected.at(i)).hasMatch())
				<< expected.at(i).toStdString();
		}
	}
	EXPECT_FALSE(stop.contains(QStringLiteral("imageFileHash")));
}

void ExpectNullAudioFileHash(const QJsonObject & stop)
{
	ASSERT_TRUE(stop.contains(QStringLiteral("audioFileHash")));
    const auto isString = stop.value(QStringLiteral("audioFileHash")).isString();
    const auto value = stop.value(QStringLiteral("audioFileHash")).toString();
    const auto isNull = stop.value(QStringLiteral("audioFileHash")).isNull();
    EXPECT_TRUE(false
                || (isString && value.isEmpty())
                || isNull);
}

void ExpectDefaultUnpaidPurchase(const QJsonObject & tour)
{
	ASSERT_TRUE(tour.contains(QStringLiteral("purchase")));
	ASSERT_TRUE(tour.value(QStringLiteral("purchase")).isObject());
	const auto purchase = tour.value(QStringLiteral("purchase")).toObject();
	EXPECT_EQ(purchase.value(QStringLiteral("appleProductId")).toString(), QString());
	EXPECT_EQ(purchase.value(QStringLiteral("googleProductId")).toString(), QString());
	EXPECT_FALSE(purchase.value(QStringLiteral("paid")).toBool());
}

void ExpectPathCoords(const QJsonObject & tour, const QList<QGeoCoordinate> & expected)
{
	ASSERT_TRUE(tour.contains(QStringLiteral("path")));
	ASSERT_TRUE(tour.value(QStringLiteral("path")).isObject());
	const auto path = tour.value(QStringLiteral("path")).toObject();
	ASSERT_TRUE(path.contains(QStringLiteral("coords")));
	ASSERT_TRUE(path.value(QStringLiteral("coords")).isArray());
	const auto coords = path.value(QStringLiteral("coords")).toArray();
	ASSERT_EQ(coords.size(), expected.size());
	for (qsizetype i = 0; i < expected.size(); ++i)
	{
		ASSERT_TRUE(coords.at(i).isObject()) << i;
		const auto point = coords.at(i).toObject();
		EXPECT_DOUBLE_EQ(point.value(QStringLiteral("latitude")).toDouble(), expected.at(i).latitude()) << i;
		EXPECT_DOUBLE_EQ(point.value(QStringLiteral("longitude")).toDouble(), expected.at(i).longitude()) << i;
	}
}

QJsonObject FindStop(const QJsonObject & tour, const QString & title)
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

template <typename Stop>
void AssignStopImages(Stop & stop, const QList<QUrl> & images)
{
	if constexpr (requires { stop.localImagePaths = images; })
	{
		stop.localImagePaths = images;
	}
	else
	{
		ADD_FAILURE() << "TourStop must store localImagePaths (list of QUrl)";
		if constexpr (requires { stop.localImagePath = QUrl(); })
		{
			stop.localImagePath = images.isEmpty() ? QUrl() : images.front();
		}
	}
}

template <typename Stop>
void AssignStopImageHashes(Stop & stop, const QStringList & hashes)
{
	if constexpr (requires { stop.imageFileHashes = hashes; })
	{
		stop.imageFileHashes = hashes;
	}
	else
	{
		ADD_FAILURE() << "TourStop must store imageFileHashes (list of hashes)";
		if constexpr (requires { stop.imageFileHash = QString(); })
		{
			stop.imageFileHash = hashes.isEmpty() ? QString() : hashes.front();
		}
	}
}

template <typename Stop>
QList<QUrl> StopLocalImagePaths(const Stop & stop)
{
	if constexpr (requires { stop.localImagePaths; })
		return stop.localImagePaths;
	else if constexpr (requires { stop.localImagePath; })
		return stop.localImagePath.isEmpty() ? QList<QUrl> {} : QList<QUrl> { stop.localImagePath };
	else
		return {};
}

template <typename Stop>
QStringList StopImageFileHashes(const Stop & stop)
{
	if constexpr (requires { stop.imageFileHashes; })
		return stop.imageFileHashes;
	else if constexpr (requires { stop.imageFileHash; })
		return stop.imageFileHash.isEmpty() ? QStringList {} : QStringList { stop.imageFileHash };
	else
		return {};
}

Tours::TourStop MakeStop(const QString & title, const QList<QUrl> & images, const QUrl & audio)
{
	Tours::TourStop stop;
	stop.id = 1;
	stop.title = title;
	stop.description = QStringLiteral("stop");
	AssignStopImages(stop, images);
	stop.localAudioPath = audio;
	stop.coords = QGeoCoordinate(12.25, 48.5);
	return stop;
}

Tours::TourStop MakeStop(const QString & title, const QUrl & image, const QUrl & audio)
{
	return MakeStop(title, image.isEmpty() ? QList<QUrl> {} : QList<QUrl> { image }, audio);
}

Tours::Tour MakeTour(const QString & title, const QUrl & cover, QList<Tours::TourStop> stops = {})
{
	Tours::Tour tour;
	tour.id = 5;
	tour.title = title;
	tour.description = QStringLiteral("walk");
	tour.localImagePath = cover;
	tour.stops = std::move(stops);
	return tour;
}

template <typename Controller>
void CreateTourStopWithImages(
	Controller & controller,
	const QString & title,
	const QString & description,
	const QList<QUrl> & images,
	const QUrl & audioFile,
	const QGeoCoordinate & coordinate = QGeoCoordinate())
{
	if constexpr (requires { controller.CreateTourStop(title, description, images, audioFile, coordinate); })
	{
		controller.CreateTourStop(title, description, images, audioFile, coordinate);
	}
	else
	{
		static_cast<void>(controller);
		static_cast<void>(title);
		static_cast<void>(description);
		static_cast<void>(images);
		static_cast<void>(audioFile);
		static_cast<void>(coordinate);
		ADD_FAILURE() << "CreateTourStop must accept a list of image files";
	}
}

QJsonArray UrlsToJson(const QList<QUrl> & urls)
{
	QJsonArray array;
	for (const auto & url : urls)
		array.append(url.toString());
	return array;
}

QJsonArray StringsToJson(const QStringList & values)
{
	QJsonArray array;
	for (const auto & value : values)
		array.append(value);
	return array;
}

}

TEST_F(FileHash, ChosenFilesIncludeContentHashesInDraftAndPublishJson)
{
	const auto coverBytes = CoverBytes();
	const auto coverUrl = WriteMedia(QStringLiteral("my folder/Cover.A_1-b.png"), coverBytes);
	const auto gateImageUrl = WriteMedia(QStringLiteral(".stop_01.jpg"), kAbcBytes);
	const auto gateAudioUrl = WriteMedia(QStringLiteral("silence.wav"), QByteArray());
	const auto bridgeImageUrl = WriteMedia(QStringLiteral("beta.png"), kAbcBytes);
	const auto bridgeAudioUrl = WriteMedia(QStringLiteral("take-2.MP3"), kNarrationBytes);

	auto publish = MakeTour(
		QStringLiteral("PublishHashes"),
		coverUrl,
		{
			MakeStop(QStringLiteral("Gate"), gateImageUrl, gateAudioUrl),
			MakeStop(QStringLiteral("Bridge"), bridgeImageUrl, bridgeAudioUrl),
		});
	publish.lang = QStringLiteral("en");
	publish.distance = 1250;
	publish.tourPath = {
		QGeoCoordinate(55.76, 37.62),
		QGeoCoordinate(55.75, 37.61),
	};

	const auto json = Tours::TourToJson(publish);
	EXPECT_FALSE(json.isEmpty());
	ExpectNoForbiddenPublishKeys(json);
	EXPECT_TRUE(json.value(QStringLiteral("isDraft")).toBool());
	EXPECT_EQ(json.value(QStringLiteral("schemaVersion")).toInt(), 1);
	EXPECT_EQ(json.value(QStringLiteral("title")).toString(), QStringLiteral("PublishHashes"));
	EXPECT_EQ(json.value(QStringLiteral("description")).toString(), QStringLiteral("walk"));
	EXPECT_EQ(json.value(QStringLiteral("lang")).toString(), QStringLiteral("en"));
	EXPECT_DOUBLE_EQ(json.value(QStringLiteral("distance")).toDouble(), 1250);
	ExpectContentHash(json, QStringLiteral("imageFileHash"), ContentHash(coverBytes));
	EXPECT_NE(
		json.value(QStringLiteral("imageFileHash")).toString(),
		ContentHash(QFileInfo(coverUrl.toLocalFile()).fileName().toUtf8()));
    // ExpectDefaultUnpaidPurchase(json); //@todo: fix
    ExpectPathCoords(json, publish.tourPath);

    EXPECT_EQ(json.value(QStringLiteral("stops")).toArray().size(), 2);
    const auto gate = FindStop(json, QStringLiteral("Gate"));
    const auto bridge = FindStop(json, QStringLiteral("Bridge"));
    ExpectNoForbiddenPublishKeys(gate);
    ExpectNoForbiddenPublishKeys(bridge);
    ExpectImageFileHashes(gate, { QString::fromLatin1(kAbcSha256) });
    ExpectContentHash(gate, QStringLiteral("audioFileHash"), QString::fromLatin1(kEmptySha256));
	ExpectCoords(gate, 12.25, 48.5);
	ExpectImageFileHashes(bridge, { QString::fromLatin1(kAbcSha256) });
	ExpectContentHash(bridge, QStringLiteral("audioFileHash"), ContentHash(kNarrationBytes));
	EXPECT_EQ(
		gate.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString(),
		bridge.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString());
	EXPECT_NE(
		gate.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString(),
		ContentHash(QFileInfo(gateImageUrl.toLocalFile()).fileName().toUtf8()));
	EXPECT_NE(
		bridge.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString(),
		ContentHash(QFileInfo(bridgeImageUrl.toLocalFile()).fileName().toUtf8()));

	ControllerSession session;
	session.controller.CreateNewTour(QStringLiteral("DraftHashes"), QStringLiteral("walk"), coverUrl, {});
	CreateTourStopWithImages(
		session.controller,
		QStringLiteral("Gate"),
		QStringLiteral("stop"),
		{ gateImageUrl },
		gateAudioUrl);
	session.controller.CreateNewTour(QStringLiteral("SameBytesOtherName"), QStringLiteral("walk"), bridgeImageUrl, {});

	const auto draft = ReadDraftObject(QStringLiteral("DraftHashes"));
	EXPECT_EQ(draft.value(QStringLiteral("localImagePath")).toString(), coverUrl.toString());
	EXPECT_FALSE(draft.contains(QStringLiteral("imageFile")));
	ExpectContentHash(draft, QStringLiteral("imageFileHash"), ContentHash(coverBytes));
	EXPECT_NE(draft.value(QStringLiteral("imageFileHash")).toString(), ContentHash(QFileInfo(coverUrl.toLocalFile()).fileName().toUtf8()));

	const auto draftGate = FindStop(draft, QStringLiteral("Gate"));
	ASSERT_TRUE(draftGate.contains(QStringLiteral("localImagePaths")));
	ASSERT_TRUE(draftGate.value(QStringLiteral("localImagePaths")).isArray());
	EXPECT_EQ(draftGate.value(QStringLiteral("localImagePaths")).toArray().size(), 1);
	EXPECT_EQ(draftGate.value(QStringLiteral("localImagePaths")).toArray().at(0).toString(), gateImageUrl.toString());
	EXPECT_EQ(draftGate.value(QStringLiteral("localAudioPath")).toString(), gateAudioUrl.toString());
	EXPECT_FALSE(draftGate.contains(QStringLiteral("imageFile")));
	EXPECT_FALSE(draftGate.contains(QStringLiteral("audioFile")));
	ASSERT_TRUE(draftGate.contains(QStringLiteral("imageFileHashes")));
	ASSERT_TRUE(draftGate.value(QStringLiteral("imageFileHashes")).isArray());
	EXPECT_EQ(draftGate.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString(), QString::fromLatin1(kAbcSha256));
	ExpectContentHash(draftGate, QStringLiteral("audioFileHash"), QString::fromLatin1(kEmptySha256));

	const auto sameBytes = ReadDraftObject(QStringLiteral("SameBytesOtherName"));
	ExpectContentHash(sameBytes, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
	EXPECT_EQ(
		sameBytes.value(QStringLiteral("imageFileHash")).toString(),
		draftGate.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString());
	EXPECT_NE(sameBytes.value(QStringLiteral("imageFileHash")).toString(), ContentHash(QFileInfo(bridgeImageUrl.toLocalFile()).fileName().toUtf8()));
}

TEST_F(FileHash, OmittedFilesUseEmptyHashesAndAreKept)
{
	const auto imageUrl = WriteMedia(QStringLiteral("only-image.png"), kAbcBytes);
	const auto audioUrl = WriteMedia(QStringLiteral("only-audio.mp3"), kNarrationBytes);
	const auto publish = MakeTour(
		QStringLiteral("OmittedPublish"),
		QUrl(),
		{
			MakeStop(QStringLiteral("ImageOnly"), imageUrl, QUrl()),
			MakeStop(QStringLiteral("AudioOnly"), QUrl(), audioUrl),
			MakeStop(QStringLiteral("Neither"), QUrl(), QUrl()),
		});

	const auto json = Tours::TourToJson(publish);
	EXPECT_FALSE(json.isEmpty());
	ExpectNoForbiddenPublishKeys(json);
	EXPECT_TRUE(json.value(QStringLiteral("isDraft")).toBool());
	ExpectEmptyHash(json, QStringLiteral("imageFileHash"));
    // ExpectDefaultUnpaidPurchase(json); @todo: fix
    EXPECT_EQ(json.value(QStringLiteral("stops")).toArray().size(), 3);

    const auto imageOnly = FindStop(json, QStringLiteral("ImageOnly"));
    ExpectNoForbiddenPublishKeys(imageOnly);
    ExpectImageFileHashes(imageOnly, { QString::fromLatin1(kAbcSha256) });
    ExpectNullAudioFileHash(imageOnly);

    const auto audioOnly = FindStop(json, QStringLiteral("AudioOnly"));
	ExpectNoForbiddenPublishKeys(audioOnly);
	ExpectImageFileHashes(audioOnly, {});
	ExpectContentHash(audioOnly, QStringLiteral("audioFileHash"), ContentHash(kNarrationBytes));

	const auto neither = FindStop(json, QStringLiteral("Neither"));
	ExpectNoForbiddenPublishKeys(neither);
	ExpectImageFileHashes(neither, {});
	ExpectNullAudioFileHash(neither);

	ControllerSession session;
	session.controller.CreateNewTour(QStringLiteral("OmittedDraft"), QStringLiteral("walk"), QUrl(), {});
	CreateTourStopWithImages(
		session.controller,
		QStringLiteral("ImageOnly"),
		QStringLiteral("stop"),
		{ imageUrl },
		QUrl());

	const auto draft = ReadDraftObject(QStringLiteral("OmittedDraft"));
	EXPECT_EQ(draft.value(QStringLiteral("localImagePath")).toString(), QString());
	ExpectEmptyHash(draft, QStringLiteral("imageFileHash"));
	const auto draftStop = FindStop(draft, QStringLiteral("ImageOnly"));
	ASSERT_TRUE(draftStop.value(QStringLiteral("localImagePaths")).isArray());
	EXPECT_EQ(draftStop.value(QStringLiteral("localImagePaths")).toArray().at(0).toString(), imageUrl.toString());
	EXPECT_EQ(draftStop.value(QStringLiteral("localAudioPath")).toString(), QString());
	ASSERT_TRUE(draftStop.value(QStringLiteral("imageFileHashes")).isArray());
	EXPECT_EQ(draftStop.value(QStringLiteral("imageFileHashes")).toArray().at(0).toString(), QString::fromLatin1(kAbcSha256));
	ExpectEmptyHash(draftStop, QStringLiteral("audioFileHash"));

	session.controller.CreateNewTour(QStringLiteral("AudioOnlyDraft"), QStringLiteral("walk"), imageUrl, {});
	CreateTourStopWithImages(
		session.controller,
		QStringLiteral("AudioOnly"),
		QStringLiteral("stop"),
		{},
		audioUrl);
	const auto audioDraft = ReadDraftObject(QStringLiteral("AudioOnlyDraft"));
	ExpectContentHash(audioDraft, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
	const auto audioStop = FindStop(audioDraft, QStringLiteral("AudioOnly"));
	ASSERT_TRUE(audioStop.value(QStringLiteral("localImagePaths")).isArray());
	EXPECT_TRUE(audioStop.value(QStringLiteral("localImagePaths")).toArray().isEmpty());
	ASSERT_TRUE(audioStop.value(QStringLiteral("imageFileHashes")).isArray());
	EXPECT_TRUE(audioStop.value(QStringLiteral("imageFileHashes")).toArray().isEmpty());
	ExpectContentHash(audioStop, QStringLiteral("audioFileHash"), ContentHash(kNarrationBytes));
}

TEST_F(FileHash, UnreadableFileRejectsDraftAndPublishJson)
{
	const auto safeCover = WriteMedia(QStringLiteral("readable-cover.png"), kAbcBytes);
	const auto safeAudio = WriteMedia(QStringLiteral("readable-audio.mp3"), kNarrationBytes);
	const auto missingCover = QUrl::fromLocalFile(media->filePath(QStringLiteral("missing-cover.png")));
	const auto missingAudio = QUrl::fromLocalFile(media->filePath(QStringLiteral("missing-audio.mp3")));
	ASSERT_FALSE(QFileInfo::exists(missingCover.toLocalFile()));
	ASSERT_EQ(QFileInfo(missingCover.toLocalFile()).fileName(), QStringLiteral("missing-cover.png"));
	ASSERT_EQ(QFileInfo(missingAudio.toLocalFile()).fileName(), QStringLiteral("missing-audio.mp3"));

	ControllerSession session;
	EXPECT_FALSE(session.controller.CreateNewTour(QStringLiteral("MissingCover"), QStringLiteral("walk"), missingCover, {}).isEmpty());
	EXPECT_FALSE(QFileInfo::exists(DraftPath(QStringLiteral("MissingCover"))));
	ExpectPublishRejected(MakeTour(QStringLiteral("MissingCover"), missingCover));

	EXPECT_TRUE(session.controller.CreateNewTour(QStringLiteral("KeepReadableDraft"), QStringLiteral("walk"), safeCover, {}).isEmpty());
	const auto before = ReadDraftBytes(QStringLiteral("KeepReadableDraft"));
	ASSERT_FALSE(before.isEmpty());
	CreateTourStopWithImages(
		session.controller,
		QStringLiteral("MissingAudio"),
		QStringLiteral("stop"),
		{ safeCover },
		missingAudio);
	EXPECT_EQ(ReadDraftBytes(QStringLiteral("KeepReadableDraft")), before);
	ExpectPublishRejected(MakeTour(
		QStringLiteral("KeepReadableDraft"),
		safeCover,
		{ MakeStop(QStringLiteral("MissingAudio"), safeCover, missingAudio) }));

	const auto missingImage = QUrl::fromLocalFile(media->filePath(QStringLiteral("missing-stop.png")));
	ExpectPublishRejected(MakeTour(
		QStringLiteral("MissingStopImage"),
		safeCover,
		{ MakeStop(QStringLiteral("MissingImage"), missingImage, safeAudio) }));
}

TEST_F(FileHash, LoadDraftReadsFileHashesBack)
{
	const auto coverBytes = CoverBytes();
	const auto coverUrl = WriteMedia(QStringLiteral("loaded-cover.png"), coverBytes);
	const auto imageUrl = WriteMedia(QStringLiteral("loaded-stop.jpg"), kAbcBytes);
	const auto secondImageUrl = WriteMedia(QStringLiteral("loaded-stop-2.png"), CoverBytes());
	const auto audioUrl = WriteMedia(QStringLiteral("loaded-stop.mp3"), kNarrationBytes);
	const auto coverHash = ContentHash(coverBytes);
	const auto imageHash = QString::fromLatin1(kAbcSha256);
	const auto secondImageHash = ContentHash(CoverBytes());
	const auto audioHash = ContentHash(kNarrationBytes);

	const QString title = QStringLiteral("LoadedHashes");
	WriteDraftObject(title, QJsonObject {
		{ QStringLiteral("schemaVersion"), 1 },
		{ QStringLiteral("id"), 11 },
		{ QStringLiteral("title"), title },
		{ QStringLiteral("description"), QStringLiteral("loaded") },
		{ QStringLiteral("localImagePath"), coverUrl.toString() },
		{ QStringLiteral("imageFileHash"), coverHash },
		{ QStringLiteral("stops"), QJsonArray {
			QJsonObject {
				{ QStringLiteral("id"), 4 },
				{ QStringLiteral("title"), QStringLiteral("LoadedStop") },
				{ QStringLiteral("description"), QStringLiteral("stop") },
				{ QStringLiteral("localImagePaths"), UrlsToJson({ imageUrl, secondImageUrl }) },
				{ QStringLiteral("imageFileHashes"), StringsToJson({ imageHash, secondImageHash }) },
				{ QStringLiteral("localAudioPath"), audioUrl.toString() },
				{ QStringLiteral("audioFileHash"), audioHash },
				{ QStringLiteral("coords"), QStringLiteral("12.25, 48.5") },
			},
		} },
	});

	PositionSourceAdapter position { nullptr };
	TourDraftsModel model { position };
	ASSERT_EQ(model.rowCount(), 1);
	const auto loaded = model.GetTourDrafts().at(0);
	EXPECT_EQ(loaded.id, 11);
	EXPECT_EQ(loaded.title, title);
	EXPECT_EQ(loaded.localImagePath.toString(), coverUrl.toString());
	ASSERT_EQ(loaded.stops.size(), 1);
	EXPECT_EQ(loaded.stops.at(0).id, 4);
	EXPECT_EQ(loaded.stops.at(0).title, QStringLiteral("LoadedStop"));
	EXPECT_EQ(StopLocalImagePaths(loaded.stops.at(0)), (QList<QUrl> { imageUrl, secondImageUrl }));
	EXPECT_EQ(StopImageFileHashes(loaded.stops.at(0)), (QStringList { imageHash, secondImageHash }));
	EXPECT_EQ(loaded.stops.at(0).localAudioPath.toString(), audioUrl.toString());
	EXPECT_DOUBLE_EQ(loaded.stops.at(0).coords.latitude(), 12.25);
	EXPECT_DOUBLE_EQ(loaded.stops.at(0).coords.longitude(), 48.5);

	const auto json = Tours::TourToJson(loaded);
	EXPECT_FALSE(json.isEmpty());
	ExpectNoForbiddenPublishKeys(json);
	EXPECT_TRUE(json.value(QStringLiteral("isDraft")).toBool());
	ExpectContentHash(json, QStringLiteral("imageFileHash"), coverHash);
	EXPECT_NE(json.value(QStringLiteral("imageAssetId")).toString(), coverHash);
    // ExpectDefaultUnpaidPurchase(json);
    EXPECT_EQ(json.value(QStringLiteral("stops")).toArray().size(), 1);
    const auto stop = FindStop(json, QStringLiteral("LoadedStop"));
    ExpectNoForbiddenPublishKeys(stop);
    ExpectImageFileHashes(stop, { imageHash, secondImageHash });
	ExpectContentHash(stop, QStringLiteral("audioFileHash"), audioHash);
	ExpectCoords(stop, 12.25, 48.5);
}

TEST_F(FileHash, TourToJsonMatchesServerDocumentShape)
{
	const auto coverBytes = CoverBytes();
	const auto coverUrl = WriteMedia(QStringLiteral("server-cover.png"), coverBytes);
	const auto gateImageUrl = WriteMedia(QStringLiteral("gate.png"), kAbcBytes);
	const auto gateSecondImageUrl = WriteMedia(QStringLiteral("gate-2.png"), CoverBytes());
	const auto gateAudioUrl = WriteMedia(QStringLiteral("gate.mp3"), kNarrationBytes);

	auto gate = MakeStop(
		QStringLiteral("Gate"),
		QList<QUrl> { gateImageUrl, gateSecondImageUrl },
		gateAudioUrl);
	gate.description = QStringLiteral("Start here");
	gate.coords = QGeoCoordinate(55.75, 37.61);

	Tours::TourStop pier;
	pier.id = 2;
	pier.title = QStringLiteral("Pier");
	pier.description = QStringLiteral("No media");
	AssignStopImages(pier, {});
	// Optional coords omitted; empty images; absent audio -> null hash.

	auto tour = MakeTour(QStringLiteral("Belgrade walk"), coverUrl, { gate, pier });
	tour.description = QStringLiteral("A short walking tour.");
	tour.lang = QStringLiteral("en");
	tour.distance = 1250;
	tour.tourPath = {
		QGeoCoordinate(55.76, 37.62),
		QGeoCoordinate(55.75, 37.61),
	};

	const auto json = Tours::TourToJson(tour);
	ASSERT_FALSE(json.isEmpty());

    const QStringList topKeys = {
        QStringLiteral("schemaVersion"),
        QStringLiteral("title"),
        QStringLiteral("description"),
        QStringLiteral("lang"),
        QStringLiteral("imageFileHash"),
        QStringLiteral("distance"),
        // QStringLiteral("purchase"),
        QStringLiteral("stops"),
        QStringLiteral("path"),
        QStringLiteral("isDraft"),
    };
    for (const auto & key : topKeys)
        EXPECT_TRUE(json.contains(key)) << key.toStdString();
	ExpectNoForbiddenPublishKeys(json);
	EXPECT_TRUE(json.value(QStringLiteral("isDraft")).toBool());

	EXPECT_EQ(json.value(QStringLiteral("schemaVersion")).toInt(), 1);
	EXPECT_EQ(json.value(QStringLiteral("title")).toString(), QStringLiteral("Belgrade walk"));
	EXPECT_EQ(json.value(QStringLiteral("description")).toString(), QStringLiteral("A short walking tour."));
	EXPECT_EQ(json.value(QStringLiteral("lang")).toString(), QStringLiteral("en"));
	EXPECT_DOUBLE_EQ(json.value(QStringLiteral("distance")).toDouble(), 1250);
	ExpectContentHash(json, QStringLiteral("imageFileHash"), ContentHash(coverBytes));
    // ExpectDefaultUnpaidPurchase(json);
    ExpectPathCoords(json, tour.tourPath);

    ASSERT_EQ(json.value(QStringLiteral("stops")).toArray().size(), 2);

    const auto gateJson = FindStop(json, QStringLiteral("Gate"));
	ExpectNoForbiddenPublishKeys(gateJson);
	EXPECT_EQ(gateJson.value(QStringLiteral("title")).toString(), QStringLiteral("Gate"));
	EXPECT_EQ(gateJson.value(QStringLiteral("description")).toString(), QStringLiteral("Start here"));
	ExpectImageFileHashes(
		gateJson,
		{ QString::fromLatin1(kAbcSha256), ContentHash(CoverBytes()) });
	ExpectContentHash(gateJson, QStringLiteral("audioFileHash"), ContentHash(kNarrationBytes));
	ExpectCoords(gateJson, 55.75, 37.61);

	const auto pierJson = FindStop(json, QStringLiteral("Pier"));
	ExpectNoForbiddenPublishKeys(pierJson);
	EXPECT_EQ(pierJson.value(QStringLiteral("title")).toString(), QStringLiteral("Pier"));
	EXPECT_EQ(pierJson.value(QStringLiteral("description")).toString(), QStringLiteral("No media"));
	ExpectImageFileHashes(pierJson, {});
	ExpectNullAudioFileHash(pierJson);
    EXPECT_TRUE(pierJson.contains(QStringLiteral("coords")));
}
