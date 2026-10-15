#include <cstring>
#include <vector>

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaObject>
#include <QMetaProperty>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <gtest/gtest.h>

#include "App/Tours/DisplayTours.h"

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

constexpr auto kNonEmptyStopAudioUrl = "https://pastviewer.com/media/stops/gate.mp3";
constexpr auto kTourAudioUrl = "https://pastviewer.com/media/tours/overview.mp3";

enum class AudioField
{
	NonEmpty,
	JsonNull,
	Missing,
	EmptyString,
};

void EnsureCoreApplication()
{
	if (QCoreApplication::instance())
		return;

	// Leak the application. QNetworkAccessManager leaves process-lifetime
	// objects that crash if the application is destroyed first.
	static int argc = 1;
	static char executableName[] = "PastViewerTests";
	static char * argv[] = { executableName, nullptr };
	new QCoreApplication(argc, argv);
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

QJsonObject StopJson(const QString & title, AudioField audioField)
{
	QJsonObject stop {
		{ QStringLiteral("title"), title },
		{ QStringLiteral("description"), QStringLiteral("Stop description") },
		{ QStringLiteral("imageUrl"), QStringLiteral("https://pastviewer.com/media/stops/image.jpg") },
		{ QStringLiteral("coords"),
			QJsonObject {
				{ QStringLiteral("latitude"), 52.37 },
				{ QStringLiteral("longitude"), 4.89 },
			} },
	};

	switch (audioField)
	{
		case AudioField::NonEmpty:
			stop.insert(QStringLiteral("audioUrl"), QString(kNonEmptyStopAudioUrl));
			break;
		case AudioField::JsonNull:
			stop.insert(QStringLiteral("audioUrl"), QJsonValue::Null);
			break;
		case AudioField::Missing:
			break;
		case AudioField::EmptyString:
			stop.insert(QStringLiteral("audioUrl"), QStringLiteral(""));
			break;
	}

	return stop;
}

QByteArray PublishedTourPayload()
{
	const QJsonArray stops {
		StopJson(QStringLiteral("NonEmptyAudio"), AudioField::NonEmpty),
		StopJson(QStringLiteral("NullAudio"), AudioField::JsonNull),
		StopJson(QStringLiteral("MissingAudio"), AudioField::Missing),
		StopJson(QStringLiteral("EmptyAudio"), AudioField::EmptyString),
	};

	const QJsonObject tour {
		{ QStringLiteral("title"), QStringLiteral("Canal Walk") },
		{ QStringLiteral("description"), QStringLiteral("A published tour") },
		{ QStringLiteral("imageUrl"), QStringLiteral("https://pastviewer.com/media/tours/cover.jpg") },
		{ QStringLiteral("audioUrl"), QString(kTourAudioUrl) },
		{ QStringLiteral("distance"), 1200 },
		{ QStringLiteral("stopCount"), static_cast<int>(stops.size()) },
		{ QStringLiteral("path"), QJsonObject { { QStringLiteral("coords"), QJsonArray {} } } },
		{ QStringLiteral("stops"), stops },
	};

	return QJsonDocument(QJsonObject { { QStringLiteral("items"), QJsonArray { tour } } }).toJson(QJsonDocument::Compact);
}

QString StopAudioUrl(const DisplayTourStop & stop)
{
	const QMetaObject & metaObject = DisplayTourStop::staticMetaObject;
	const int propertyIndex = metaObject.indexOfProperty("audioUrl");
	if (propertyIndex < 0)
		return {};

	return metaObject.property(propertyIndex).readOnGadget(&stop).toString();
}

const DisplayTourStop * FindStop(const QList<DisplayTourStop> & stops, const char * title)
{
	for (const auto & stop : stops)
	{
		if (stop.title == QLatin1String(title))
			return &stop;
	}
	return nullptr;
}

QList<DisplayTourStop> LoadPublishedStops()
{
	EnsureCoreApplication();

	const auto payload = PublishedTourPayload();
	const auto root = QJsonDocument::fromJson(payload).object();
	const auto tour = root.value(QStringLiteral("items")).toArray().at(0).toObject();
	const auto stopsJson = tour.value(QStringLiteral("stops")).toArray();
	EXPECT_EQ(tour.value(QStringLiteral("audioUrl")).toString().toStdString(), kTourAudioUrl);
	EXPECT_EQ(stopsJson.at(0).toObject().value(QStringLiteral("audioUrl")).toString().toStdString(), kNonEmptyStopAudioUrl);
	EXPECT_TRUE(stopsJson.at(1).toObject().value(QStringLiteral("audioUrl")).isNull());
	EXPECT_FALSE(stopsJson.at(2).toObject().contains(QStringLiteral("audioUrl")));
	EXPECT_TRUE(stopsJson.at(3).toObject().value(QStringLiteral("audioUrl")).isString());
	EXPECT_TRUE(stopsJson.at(3).toObject().value(QStringLiteral("audioUrl")).toString().isEmpty());

	AvailableToursModel model;
	QObject::disconnect(&model.m_impl->networkManager, nullptr, &model, nullptr);

	auto * reply = new JsonBodyReply(payload);
	model.OnNetworkReplyFinished(reply);

	if (model.rowCount() != 1)
	{
		ADD_FAILURE() << "published tour payload was not loaded: " << payload.toStdString();
		return {};
	}

	const auto value = model.data(model.index(0, 0), AvailableToursModel::Stops);
	if (!value.canConvert<QList<DisplayTourStop>>())
	{
		ADD_FAILURE() << "stops role is not a list of display stops";
		return {};
	}

	return value.value<QList<DisplayTourStop>>();
}

class StopAudio : public ::testing::Test
{
protected:
	static void SetUpTestSuite()
	{
		EnsureCoreApplication();
	}
};

}

TEST_F(StopAudio, KeepsNonEmptyAudioUrl)
{
	const auto stops = LoadPublishedStops();
	ASSERT_EQ(stops.size(), 4);

	const auto * stop = FindStop(stops, "NonEmptyAudio");
	ASSERT_NE(stop, nullptr);
	EXPECT_EQ(StopAudioUrl(*stop).toStdString(), std::string(kNonEmptyStopAudioUrl));
}

TEST_F(StopAudio, JsonNullBecomesEmptyAudioUrl)
{
	const auto stops = LoadPublishedStops();
	ASSERT_EQ(stops.size(), 4);

	const auto * stop = FindStop(stops, "NullAudio");
	ASSERT_NE(stop, nullptr);
	EXPECT_EQ(StopAudioUrl(*stop).toStdString(), std::string());
}

TEST_F(StopAudio, MissingFieldBecomesEmptyAudioUrl)
{
	const auto stops = LoadPublishedStops();
	ASSERT_EQ(stops.size(), 4);

	const auto * stop = FindStop(stops, "MissingAudio");
	ASSERT_NE(stop, nullptr);
	EXPECT_EQ(StopAudioUrl(*stop).toStdString(), std::string());
}

TEST_F(StopAudio, EmptyStringBecomesEmptyAudioUrl)
{
	const auto stops = LoadPublishedStops();
	ASSERT_EQ(stops.size(), 4);

	const auto * stop = FindStop(stops, "EmptyAudio");
	ASSERT_NE(stop, nullptr);
	EXPECT_EQ(StopAudioUrl(*stop).toStdString(), std::string());
}
