#include <memory>
#include <ostream>
#include <utility>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
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

	// Leak the application. TourController's QNetworkAccessManager leaves
	// process-lifetime QObjects that crash if it is destroyed first.
	static int argc = 1;
	static char executableName[] = "PastViewerTests";
	static char * argv[] = { executableName, nullptr };
	new QCoreApplication(argc, argv);
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
	EXPECT_TRUE(Tours::TourToJson(tour, true).isEmpty());
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

Tours::TourStop MakeStop(const QString & title, const QUrl & image, const QUrl & audio)
{
	Tours::TourStop stop;
	stop.id = 1;
	stop.title = title;
	stop.description = QStringLiteral("stop");
	stop.localImagePath = image;
	stop.localAudioPath = audio;
	stop.coords = QGeoCoordinate(12.25, 48.5);
	return stop;
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

}

TEST_F(FileHash, ChosenFilesIncludeContentHashesInDraftAndPublishJson)
{
	const auto coverBytes = CoverBytes();
	const auto coverUrl = WriteMedia(QStringLiteral("my folder/Cover.A_1-b.png"), coverBytes);
	const auto gateImageUrl = WriteMedia(QStringLiteral(".stop_01.jpg"), kAbcBytes);
	const auto gateAudioUrl = WriteMedia(QStringLiteral("silence.wav"), QByteArray());
	const auto bridgeImageUrl = WriteMedia(QStringLiteral("beta.png"), kAbcBytes);
	const auto bridgeAudioUrl = WriteMedia(QStringLiteral("take-2.MP3"), kNarrationBytes);

	const auto publish = MakeTour(
		QStringLiteral("PublishHashes"),
		coverUrl,
		{
			MakeStop(QStringLiteral("Gate"), gateImageUrl, gateAudioUrl),
			MakeStop(QStringLiteral("Bridge"), bridgeImageUrl, bridgeAudioUrl),
		});

	for (const auto forPublish : { false, true })
	{
		SCOPED_TRACE(forPublish);
		const auto json = Tours::TourToJson(publish, forPublish);
		EXPECT_FALSE(json.isEmpty());
		EXPECT_EQ(json.value(QStringLiteral("isDraft")).toBool(), !forPublish);
		EXPECT_EQ(json.value(QStringLiteral("imageFile")).toString(), coverUrl.toString());
		EXPECT_FALSE(json.contains(QStringLiteral("localImagePath")));
		ExpectContentHash(json, QStringLiteral("imageFileHash"), ContentHash(coverBytes));
		EXPECT_NE(json.value(QStringLiteral("imageFileHash")).toString(), ContentHash(QFileInfo(coverUrl.toLocalFile()).fileName().toUtf8()));

		EXPECT_EQ(json.value(QStringLiteral("stops")).toArray().size(), 2);
		const auto gate = FindStop(json, QStringLiteral("Gate"));
		const auto bridge = FindStop(json, QStringLiteral("Bridge"));
		EXPECT_EQ(gate.value(QStringLiteral("imageFile")).toString(), gateImageUrl.toString());
		EXPECT_EQ(gate.value(QStringLiteral("audioFile")).toString(), gateAudioUrl.toString());
		EXPECT_FALSE(gate.contains(QStringLiteral("localImagePath")));
		EXPECT_FALSE(gate.contains(QStringLiteral("localAudioPath")));
		ExpectContentHash(gate, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
		ExpectContentHash(gate, QStringLiteral("audioFileHash"), QString::fromLatin1(kEmptySha256));
		EXPECT_EQ(bridge.value(QStringLiteral("imageFile")).toString(), bridgeImageUrl.toString());
		EXPECT_EQ(bridge.value(QStringLiteral("audioFile")).toString(), bridgeAudioUrl.toString());
		ExpectContentHash(bridge, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
		ExpectContentHash(bridge, QStringLiteral("audioFileHash"), ContentHash(kNarrationBytes));
		EXPECT_EQ(gate.value(QStringLiteral("imageFileHash")).toString(), bridge.value(QStringLiteral("imageFileHash")).toString());
		EXPECT_NE(gate.value(QStringLiteral("imageFileHash")).toString(), ContentHash(QFileInfo(gateImageUrl.toLocalFile()).fileName().toUtf8()));
		EXPECT_NE(bridge.value(QStringLiteral("imageFileHash")).toString(), ContentHash(QFileInfo(bridgeImageUrl.toLocalFile()).fileName().toUtf8()));
	}

	ControllerSession session;
	session.controller.CreateNewTour(QStringLiteral("DraftHashes"), QStringLiteral("walk"), coverUrl);
	session.controller.CreateTourStop(QStringLiteral("Gate"), QStringLiteral("stop"), gateImageUrl, gateAudioUrl);
	session.controller.CreateNewTour(QStringLiteral("SameBytesOtherName"), QStringLiteral("walk"), bridgeImageUrl);

	const auto draft = ReadDraftObject(QStringLiteral("DraftHashes"));
	EXPECT_EQ(draft.value(QStringLiteral("localImagePath")).toString(), coverUrl.toString());
	EXPECT_FALSE(draft.contains(QStringLiteral("imageFile")));
	ExpectContentHash(draft, QStringLiteral("imageFileHash"), ContentHash(coverBytes));
	EXPECT_NE(draft.value(QStringLiteral("imageFileHash")).toString(), ContentHash(QFileInfo(coverUrl.toLocalFile()).fileName().toUtf8()));

	const auto gate = FindStop(draft, QStringLiteral("Gate"));
	EXPECT_EQ(gate.value(QStringLiteral("localImagePath")).toString(), gateImageUrl.toString());
	EXPECT_EQ(gate.value(QStringLiteral("localAudioPath")).toString(), gateAudioUrl.toString());
	EXPECT_FALSE(gate.contains(QStringLiteral("imageFile")));
	EXPECT_FALSE(gate.contains(QStringLiteral("audioFile")));
	ExpectContentHash(gate, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
	ExpectContentHash(gate, QStringLiteral("audioFileHash"), QString::fromLatin1(kEmptySha256));
	EXPECT_NE(gate.value(QStringLiteral("imageFileHash")).toString(), ContentHash(QFileInfo(gateImageUrl.toLocalFile()).fileName().toUtf8()));

	const auto sameBytes = ReadDraftObject(QStringLiteral("SameBytesOtherName"));
	ExpectContentHash(sameBytes, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
	EXPECT_EQ(sameBytes.value(QStringLiteral("imageFileHash")).toString(), gate.value(QStringLiteral("imageFileHash")).toString());
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

	const auto json = Tours::TourToJson(publish, true);
	EXPECT_FALSE(json.isEmpty());
	EXPECT_EQ(json.value(QStringLiteral("imageFile")).toString(), QString());
	ExpectEmptyHash(json, QStringLiteral("imageFileHash"));
	EXPECT_EQ(json.value(QStringLiteral("stops")).toArray().size(), 3);

	const auto imageOnly = FindStop(json, QStringLiteral("ImageOnly"));
	ExpectContentHash(imageOnly, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
	ExpectEmptyHash(imageOnly, QStringLiteral("audioFileHash"));

	const auto audioOnly = FindStop(json, QStringLiteral("AudioOnly"));
	ExpectEmptyHash(audioOnly, QStringLiteral("imageFileHash"));
	ExpectContentHash(audioOnly, QStringLiteral("audioFileHash"), ContentHash(kNarrationBytes));

	const auto neither = FindStop(json, QStringLiteral("Neither"));
	ExpectEmptyHash(neither, QStringLiteral("imageFileHash"));
	ExpectEmptyHash(neither, QStringLiteral("audioFileHash"));

	ControllerSession session;
	session.controller.CreateNewTour(QStringLiteral("OmittedDraft"), QStringLiteral("walk"), QUrl());
	session.controller.CreateTourStop(QStringLiteral("ImageOnly"), QStringLiteral("stop"), imageUrl, QUrl());

	const auto draft = ReadDraftObject(QStringLiteral("OmittedDraft"));
	EXPECT_EQ(draft.value(QStringLiteral("localImagePath")).toString(), QString());
	ExpectEmptyHash(draft, QStringLiteral("imageFileHash"));
	const auto draftStop = FindStop(draft, QStringLiteral("ImageOnly"));
	EXPECT_EQ(draftStop.value(QStringLiteral("localImagePath")).toString(), imageUrl.toString());
	EXPECT_EQ(draftStop.value(QStringLiteral("localAudioPath")).toString(), QString());
	ExpectContentHash(draftStop, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
	ExpectEmptyHash(draftStop, QStringLiteral("audioFileHash"));

	session.controller.CreateNewTour(QStringLiteral("AudioOnlyDraft"), QStringLiteral("walk"), imageUrl);
	session.controller.CreateTourStop(QStringLiteral("AudioOnly"), QStringLiteral("stop"), QUrl(), audioUrl);
	const auto audioDraft = ReadDraftObject(QStringLiteral("AudioOnlyDraft"));
	ExpectContentHash(audioDraft, QStringLiteral("imageFileHash"), QString::fromLatin1(kAbcSha256));
	const auto audioStop = FindStop(audioDraft, QStringLiteral("AudioOnly"));
	ExpectEmptyHash(audioStop, QStringLiteral("imageFileHash"));
	ExpectContentHash(audioStop, QStringLiteral("audioFileHash"), ContentHash(kNarrationBytes));
}

TEST_F(FileHash, UnsafeBasenameRejectsDraftAndPublishJson)
{
	const auto safeCover = WriteMedia(QStringLiteral("safe-cover.png"), kAbcBytes);
	const auto safeAudio = WriteMedia(QStringLiteral("safe-audio.mp3"), kNarrationBytes);

	const QString unsafeNames[] = {
		QStringLiteral("bad name.png"),
		QStringLiteral("price$.mp3"),
		QStringLiteral("photo(1).jpg"),
	};

	ControllerSession session;
	for (const auto & name : unsafeNames)
	{
		SCOPED_TRACE(name.toStdString());
		const auto unsafeUrl = WriteMedia(name, kAbcBytes);
		const auto title = QStringLiteral("Unsafe-%1").arg(name);
		session.controller.CreateNewTour(title, QStringLiteral("walk"), unsafeUrl);
		EXPECT_FALSE(QFileInfo::exists(DraftPath(title))) << DraftPath(title).toStdString();
		ExpectPublishRejected(MakeTour(title, unsafeUrl));

		const auto stopTitle = QStringLiteral("Stop-%1").arg(name);
		session.controller.CreateNewTour(stopTitle, QStringLiteral("walk"), safeCover);
		const auto before = ReadDraftBytes(stopTitle);
		ASSERT_FALSE(before.isEmpty());
		session.controller.CreateTourStop(QStringLiteral("BadStop"), QStringLiteral("stop"), safeCover, unsafeUrl);
		EXPECT_EQ(ReadDraftBytes(stopTitle), before);
		auto rejectedStop = MakeTour(stopTitle, safeCover, { MakeStop(QStringLiteral("BadStop"), safeCover, unsafeUrl) });
		ExpectPublishRejected(rejectedStop);
	}

	QUrl emptyBasename(QStringLiteral("file:///"));
	ASSERT_FALSE(emptyBasename.isEmpty());
	ASSERT_TRUE(emptyBasename.fileName().isEmpty());
	session.controller.CreateNewTour(QStringLiteral("EmptyBasename"), QStringLiteral("walk"), emptyBasename);
	EXPECT_FALSE(QFileInfo::exists(DraftPath(QStringLiteral("EmptyBasename"))));
	ExpectPublishRejected(MakeTour(QStringLiteral("EmptyBasename"), emptyBasename));

	const auto unsafeAudio = WriteMedia(QStringLiteral("bad audio.mp3"), kNarrationBytes);
	session.controller.CreateNewTour(QStringLiteral("KeepGoodDraft"), QStringLiteral("walk"), safeCover);
	const auto goodDraft = ReadDraftBytes(QStringLiteral("KeepGoodDraft"));
	session.controller.CreateTourStop(QStringLiteral("BadAudio"), QStringLiteral("stop"), safeCover, unsafeAudio);
	EXPECT_EQ(ReadDraftBytes(QStringLiteral("KeepGoodDraft")), goodDraft);
	ExpectPublishRejected(MakeTour(QStringLiteral("KeepGoodDraft"), safeCover, { MakeStop(QStringLiteral("BadAudio"), safeCover, unsafeAudio) }));

	const auto unsafeImage = WriteMedia(QStringLiteral("bad image.png"), kAbcBytes);
	ExpectPublishRejected(MakeTour(QStringLiteral("BadImage"), safeCover, { MakeStop(QStringLiteral("BadImage"), unsafeImage, safeAudio) }));
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
	session.controller.CreateNewTour(QStringLiteral("MissingCover"), QStringLiteral("walk"), missingCover);
	EXPECT_FALSE(QFileInfo::exists(DraftPath(QStringLiteral("MissingCover"))));
	ExpectPublishRejected(MakeTour(QStringLiteral("MissingCover"), missingCover));

	session.controller.CreateNewTour(QStringLiteral("KeepReadableDraft"), QStringLiteral("walk"), safeCover);
	const auto before = ReadDraftBytes(QStringLiteral("KeepReadableDraft"));
	ASSERT_FALSE(before.isEmpty());
	session.controller.CreateTourStop(QStringLiteral("MissingAudio"), QStringLiteral("stop"), safeCover, missingAudio);
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
	const auto audioUrl = WriteMedia(QStringLiteral("loaded-stop.mp3"), kNarrationBytes);
	const auto coverHash = ContentHash(coverBytes);
	const auto imageHash = QString::fromLatin1(kAbcSha256);
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
				{ QStringLiteral("localImagePath"), imageUrl.toString() },
				{ QStringLiteral("imageFileHash"), imageHash },
				{ QStringLiteral("localAudioPath"), audioUrl.toString() },
				{ QStringLiteral("audioFileHash"), audioHash },
				{ QStringLiteral("coords"), QStringLiteral("12.25, 48.5") },
			},
		} },
	});

	TourDraftsModel model;
	ASSERT_EQ(model.rowCount(), 1);
	const auto loaded = model.GetTourDrafts().at(0);
	EXPECT_EQ(loaded.id, 11);
	EXPECT_EQ(loaded.title, title);
	EXPECT_EQ(loaded.localImagePath.toString(), coverUrl.toString());
	EXPECT_NE(loaded.assetId, coverHash);
	ASSERT_EQ(loaded.stops.size(), 1);
	EXPECT_EQ(loaded.stops.at(0).id, 4);
	EXPECT_EQ(loaded.stops.at(0).title, QStringLiteral("LoadedStop"));
	EXPECT_EQ(loaded.stops.at(0).localImagePath.toString(), imageUrl.toString());
	EXPECT_EQ(loaded.stops.at(0).localAudioPath.toString(), audioUrl.toString());
	EXPECT_DOUBLE_EQ(loaded.stops.at(0).coords.latitude(), 12.25);
	EXPECT_DOUBLE_EQ(loaded.stops.at(0).coords.longitude(), 48.5);

	for (const auto forPublish : { false, true })
	{
		SCOPED_TRACE(forPublish);
		const auto json = Tours::TourToJson(loaded, forPublish);
		EXPECT_FALSE(json.isEmpty());
		EXPECT_EQ(json.value(QStringLiteral("imageFile")).toString(), coverUrl.toString());
		ExpectContentHash(json, QStringLiteral("imageFileHash"), coverHash);
		EXPECT_NE(json.value(QStringLiteral("imageAssetId")).toString(), coverHash);
		EXPECT_EQ(json.value(QStringLiteral("stops")).toArray().size(), 1);
		const auto stop = FindStop(json, QStringLiteral("LoadedStop"));
		EXPECT_EQ(stop.value(QStringLiteral("imageFile")).toString(), imageUrl.toString());
		EXPECT_EQ(stop.value(QStringLiteral("audioFile")).toString(), audioUrl.toString());
		ExpectContentHash(stop, QStringLiteral("imageFileHash"), imageHash);
		ExpectContentHash(stop, QStringLiteral("audioFileHash"), audioHash);
	}
}
