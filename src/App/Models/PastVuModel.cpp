#include "PastVuModel.h"

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QVariant>

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
	int cid { 0 };
	QGeoCoordinate coord;
	QString file;
	QString title;
	int bearing { 0 };
	int year { 0 };
	bool selected { false };
};

using Items = std::vector<Item>;

enum Roles
{
	// Getters
	Coordinate = Qt::UserRole + 1,
	Title,
	Photo,
	Thumbnail,
	Bearing,
	Year,

	// Setters
	Selected,
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
	std::unordered_set<int> seenCids;
};

PastVuModel::PastVuModel(QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>())
{
	const auto source = QGeoPositionInfoSource::createDefaultSource(this);
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

			QJsonParseError parserError;
			const auto jsonDoc = QJsonDocument::fromJson(response, &parserError);
			if (parserError.error != QJsonParseError::NoError)
				LOG(WARNING) << "Failed to parse JSON with error" << parserError.errorString().toStdString();

			const auto root = jsonDoc.object();
			const auto result = root.value("result").toObject();
			const auto photos = result.value("photos").toArray();

			beginInsertRows(QModelIndex(), m_impl->items.size(), m_impl->items.size() + photos.size());
			for (const QJsonValue & photo : photos)
			{
				const auto jsonObj = photo.toObject();
				const auto geo = jsonObj.value("geo").toArray(); // [lat, lon]
				const auto povDirection = jsonObj.value("dir").toString();

				const auto cid = jsonObj.value("cid").toInt();
				if (!m_impl->seenCids.contains(cid))
					m_impl->seenCids.insert(cid);
				else
					continue;

				m_impl->items.push_back({
					cid,
					{ geo.at(0).toDouble(), geo.at(1).toDouble() },
					jsonObj.value("file").toString(),
					jsonObj.value("title").toString(),
					BearingFromDirection(jsonObj.value("dir").toString()),
					jsonObj.value("year").toInt()
                });

				if (const auto item = m_impl->items.back(); item.bearing == 1)
					LOG(WARNING) << "Incorrect bearing for item: " << item.title.toStdString();
			}
			endInsertRows();
		}
		reply->deleteLater();
	});

	connect(this, &QAbstractListModel::rowsInserted, this, [&] { emit countChanged(); });
	connect(this, &QAbstractListModel::rowsRemoved, this, [&] { emit countChanged(); });
	connect(this, &QAbstractListModel::modelReset, this, [&] { emit countChanged(); });
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

	static constexpr auto fullSizeImageUrl = "https://pastvu.com/_p/a/";
	static constexpr auto thumbnailUrl = "https://pastvu.com/_p/h/";
	const auto item = m_impl->items.at(index.row());
	switch (role)
	{
		case Roles::Coordinate:
			return QVariant::fromValue(item.coord);
		case Roles::Title:
			return item.title;
		case Roles::Photo:
			return fullSizeImageUrl + item.file;
		case Roles::Thumbnail:
			return thumbnailUrl + item.file;
		case Roles::Bearing:
			return item.bearing;
		case Roles::Year:
			return item.year;
		case Roles::Selected:
			return item.selected;
		default:
			assert(false && "Unexpected role");
	}

	return {};
}

bool PastVuModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	auto & item = m_impl->items.at(index.row());
	switch (role)
	{
		case Roles::Selected:
		{
			const auto selectedItemIndices = match(this->index(0, 0), Roles::Selected, true);
			if (!selectedItemIndices.isEmpty() && selectedItemIndices.front() != index)
			{
				setData(selectedItemIndices.front(), false, Roles::Selected);
				emit dataChanged(selectedItemIndices.front(), selectedItemIndices.front(), { Roles::Selected });
			}

			item.selected = value.toBool();
			emit dataChanged(index, index, { Roles::Selected });
			return true;
		}
		default:
			assert(false && "Unexpected role");
	}
	return false;
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
		ROLENAME(Photo),
		ROLENAME(Thumbnail),
		ROLENAME(Bearing),
		ROLENAME(Year),
		ROLENAME(Selected),
	};
#undef ROLENAME
}
