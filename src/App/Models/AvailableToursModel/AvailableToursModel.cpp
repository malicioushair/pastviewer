#include "AvailableToursModel.h"

#include <cassert>
#include <format>

#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#include "glog/logging.h"

namespace {
constexpr auto URL = "https://pastviewer.com/api/v1/tours";
}

struct AvailableToursModel::Impl
{
	QNetworkAccessManager networkManager {};
    std::vector<Tour> items;
};

AvailableToursModel::AvailableToursModel(QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>())
{
	connect(&m_impl->networkManager, &QNetworkAccessManager::finished, this, &AvailableToursModel::OnNetworkReplyFinished);
	Update();
}

AvailableToursModel::~AvailableToursModel() = default;

void AvailableToursModel::OnNetworkReplyFinished(QNetworkReply * reply)
{
	if (reply->error())
	{
		LOG(ERROR) << std::format("Error occured when requested tour list: {}", reply->errorString().toStdString());
		reply->deleteLater();
		return;
	}

	LOG(INFO) << "Reply success";
	const auto body = reply->readAll();
	reply->deleteLater();

	QJsonParseError parserError;
	const auto jsonDoc = QJsonDocument::fromJson(body, &parserError);
	if (parserError.error != QJsonParseError::NoError)
	{
		LOG(WARNING) << "Failed to parse JSON with error" << parserError.errorString().toStdString();
		return;
	}

	const auto root = jsonDoc.object();
	const auto items = root.value("items").toArray();
	// The model is yet small, we can afford to reload it for now
	m_impl->items.clear();
	for (const auto & item : items)
	{
		const auto itemObj = item.toObject();
        const auto pathObj = itemObj.value("path").toObject();
        const auto pathCoords = pathObj.value("coords").toArray();
        QList<QGeoCoordinate> tourPath;
        tourPath.reserve(pathCoords.size());
        std::ranges::transform(pathCoords, std::back_inserter(tourPath), [&](decltype(pathCoords)::const_reference item) -> QGeoCoordinate {
            const auto itemObj = item.toObject();
            const auto lat = itemObj.value("latitude").toDouble();
            const auto lon = itemObj.value("longitude").toDouble();
            return { lat, lon };
        });

        const auto stops = itemObj.value("stops").toArray();
        QList<TourStop> tourStops;
        tourStops.reserve(stops.size());
        std::ranges::transform(stops, std::back_inserter(tourStops), [&, sequenceNumber = 0](decltype(stops)::const_reference item) mutable -> TourStop {
            const auto itemObj = item.toObject();

            const auto coordsObj = itemObj.value("coords").toObject();
            const auto lat = coordsObj.value("latitude").toDouble();
            const auto lon = coordsObj.value("longitude").toDouble();
            return {
                .sequenceNumber = ++sequenceNumber,
                .title = itemObj.value("title").toString(),
                .description = itemObj.value("description").toString(),
                .imageUrl = itemObj.value("imageUrl").toString(),
                .coords = { lat, lon },
            };
        });

        beginResetModel();
        m_impl->items.push_back({ .title = itemObj.value("title").toString(),
            .description = itemObj.value("description").toString(),
            .imageUrl = itemObj.value("imageUrl").toString(),
            .distance = itemObj.value("distance").toInt(),
            .stopCount = itemObj.value("stopCount").toInt(),
            .stops = tourStops,
            .tourPath = tourPath });
        endResetModel();
    }
}

void AvailableToursModel::Update()
{
	QNetworkRequest request({ URL });
	m_impl->networkManager.get(request);
}

int AvailableToursModel::rowCount(const QModelIndex & parent) const
{
	return static_cast<int>(m_impl->items.size());
}

QVariant AvailableToursModel::data(const QModelIndex & index, int role) const
{
	assert(index.isValid());

	const auto item = m_impl->items.at(index.row());
	switch (role)
	{
		case Title:
			return item.title;
		case Description:
			return item.description;
		case ImageUrl:
			return item.imageUrl;
        case Distance:
            return item.distance;
        case StopCount:
            return item.stopCount;
        case Stops:
            return QVariant::fromValue(item.stops);
        case TourPath:
            return QVariant::fromValue(item.tourPath);
        default:
            assert(false && "unknown role");
    }
    return {};
}

QHash<int, QByteArray> AvailableToursModel::roleNames() const
{
#define ROLENAME(NAME)     \
	{                      \
		Roles::NAME, #NAME \
	}
    return {
        ROLENAME(Title),
        ROLENAME(Description),
        ROLENAME(ImageUrl),
        ROLENAME(Distance),
        ROLENAME(StopCount),
        ROLENAME(Stops),
        ROLENAME(TourPath),
    };
#undef ROLENAME
}
