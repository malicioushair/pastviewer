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

#include "App/Tours/DisplayTours.h"

namespace {
constexpr auto URL = "https://pastviewer.com/api/v1/tours";
}

struct AvailableToursModel::Impl
{
	QNetworkAccessManager networkManager {};
    std::vector<DisplayTour> tours;
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
    m_impl->tours.clear();
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
        QList<DisplayTourStop> tourStops;
        tourStops.reserve(stops.size());
        std::ranges::transform(stops, std::back_inserter(tourStops), [&, sequenceNumber = 0](decltype(stops)::const_reference item) mutable -> DisplayTourStop {
            const auto itemObj = item.toObject();

            const auto coordsObj = itemObj.value("coords").toObject();
            const auto lat = coordsObj.value("latitude").toDouble();
            const auto lon = coordsObj.value("longitude").toDouble();
            return {
                .sequenceNumber = ++sequenceNumber,
                .title = itemObj.value("title").toString(),
                .description = itemObj.value("description").toString(),
                .imageUrl = itemObj.value("imageUrl").toString(),
                .audioUrl = itemObj.value("audioUrl").toString(),
                .coords = { lat, lon },
            };
        });

        const auto purchaseInfoObj = itemObj.value("purchaseInfo").toObject();
        beginResetModel();
        m_impl->tours.push_back({
            .id = itemObj.value("id").toString(),
            .title = itemObj.value("title").toString(),
            .description = itemObj.value("description").toString(),
            .imageUrl = itemObj.value("imageUrl").toString(),
            .distance = itemObj.value("distance").toInt(),
            .stopCount = itemObj.value("stopCount").toInt(),
            .stops = tourStops,
            .lang = itemObj.value("lang").toString(),
            .purchaseInfo = {
                             .paid = purchaseInfoObj.value("paid").toBool(),
                             .appleId = purchaseInfoObj.value("appleId").toString(),
                             .googleId = purchaseInfoObj.value("googleId").toString(),
                             },
            .tourPath = tourPath
        });
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
    return static_cast<int>(m_impl->tours.size());
}

QVariant AvailableToursModel::data(const QModelIndex & index, int role) const
{
	assert(index.isValid());

    const auto tour = m_impl->tours.at(index.row());
    switch (role)
    {
        case Title:
            return tour.title;
        case Description:
            return tour.description;
        case ImageUrl:
            return tour.imageUrl;
        case Distance:
            return tour.distance;
        case StopCount:
            return tour.stopCount;
        case Stops:
            return QVariant::fromValue(tour.stops);
        case TourObj:
            return QVariant::fromValue(tour);
        case Lang:
            return tour.lang;
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
        ROLENAME(TourObj),
        ROLENAME(Lang),
    };
#undef ROLENAME
}
