#include "PastVuModel.h"

#include <QGeoPositionInfoSource>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <stdexcept>

#include "glog/logging.h"

namespace {

int BearingFromDirection(const QString & direction)
{
	if (direction.isEmpty())
	{
		LOG(WARNING) << "Direction cannot be empty";
		return 1;
	}

	static constexpr std::array<std::pair<std::string_view, int>, 8> povDirectionToBearing {
		{
         { "n", 0 },
         { "ne", 45 },
         { "e", 90 },
         { "se", 135 },
         { "s", 180 },
         { "sw", 225 },
         { "w", 270 },
         { "nw", 315 },
		 }
	};

	const auto it = std::ranges::find_if(povDirectionToBearing, [&](decltype(povDirectionToBearing)::const_reference item) {
		return item.first == direction;
	});
	if (it == povDirectionToBearing.cend())
		throw std::runtime_error("Unknown direction: " + direction.toStdString());

	return it->second;
}

struct Item
{
	int cid = 0;
	QGeoCoordinate coord;
	QString file;
	QString title;
	int bearing = 0;
	int year = 0;
};

using Items = std::vector<Item>;

enum Roles
{
	Coordinate = Qt::UserRole + 1,
	Title,
	File,
	Bearing,
};

}

struct PastVuModel::Impl
{
	Impl()
		: networkManager(new QNetworkAccessManager())
	{
	}

	QNetworkAccessManager * networkManager;
	Items items;
};

PastVuModel::PastVuModel(QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>())
{
	const auto source = QGeoPositionInfoSource::createDefaultSource(nullptr);
	if (source)
	{
		connect(source, &QGeoPositionInfoSource::positionUpdated, this, [&](const QGeoPositionInfo & info) {
			const auto currentCoordinates = info.coordinate();
			const auto lat = currentCoordinates.latitude();
			const auto lon = currentCoordinates.longitude();
			const auto url = QString(R"(https://pastvu.com/api2?method=photo.giveNearestPhotos&params={"geo":[%1,%2],"limit":12,"except":228481})").arg(lat).arg(lon);
			QNetworkRequest request(url);
			m_impl->networkManager->get(request);
		});
		source->startUpdates(); // Start receiving position updates
		LOG(INFO) << "Position updates started.";
	}
	else
	{
		qDebug() << "No position source available.";
	}

	connect(m_impl->networkManager, &QNetworkAccessManager::finished, this, [&](QNetworkReply * reply) {
		LOG(INFO) << "Sending request";
		if (reply->error())
		{
			LOG(INFO) << "Reply error:" << reply->errorString().toStdString();
		}
		else
		{
			LOG(INFO) << "Reply received";
			const auto response = reply->readAll();
			LOG(INFO) << "Response:" << response.toStdString();

			QJsonParseError parserError;
			const auto jsonDoc = QJsonDocument::fromJson(response, &parserError);
			if (parserError.error != QJsonParseError::NoError)
				LOG(WARNING) << "Failed to parse JSON with error" << parserError.errorString().toStdString();

			const auto root = jsonDoc.object();
			const auto result = root.value("result").toObject();
			const auto photos = result.value("photos").toArray();

			for (const QJsonValue & photo : photos)
			{
				const QJsonObject jsonObj = photo.toObject();
				const QJsonArray geo = jsonObj.value("geo").toArray(); // [lat, lon]
				const auto povDirection = jsonObj.value("dir").toString();

				beginResetModel();
				m_impl->items.push_back({
					jsonObj.value("cid").toInt(),
					{ geo.at(0).toDouble(), geo.at(1).toDouble() },
					jsonObj.value("file").toString(),
					jsonObj.value("title").toString(),
					BearingFromDirection(jsonObj.value("dir").toString()),
					jsonObj.value("year").toInt()
                });

				if (const auto item = m_impl->items.back(); item.bearing == 1)
					LOG(WARNING) << "Incorrect bearing for item: " << item.title.toStdString();

				endResetModel();
				LOG(INFO) << m_impl->items.back().title.toStdString();
			}
		}
		reply->deleteLater();
	});
}

PastVuModel::~PastVuModel() = default;

int PastVuModel::rowCount(const QModelIndex & parent) const
{
	return m_impl->items.size();
}

QVariant PastVuModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return assert(false && "Invalid index"), QVariant();

	const auto item = m_impl->items.at(index.row());
	switch (role)
	{
		case Roles::Coordinate:
			return QVariant::fromValue(item.coord);
		case Roles::Title:
			return item.title;
		case Roles::File:
			return "https://pastvu.com/_p/h/" + item.file;
		case Roles::Bearing:
			return item.bearing;
		default:
			assert(false && "Unexpected role");
	}

	return {};
}

QHash<int, QByteArray> PastVuModel::roleNames() const
{
#define ROLENAME(NAME)     \
	{                      \
		Roles::NAME, #NAME \
	}
	return {
		ROLENAME(Coordinate),
		ROLENAME(Title),
		ROLENAME(File),
		ROLENAME(Bearing),
	};
#undef ROLENAME
}
