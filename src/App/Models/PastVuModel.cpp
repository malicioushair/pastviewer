#include "PastVuModel.h"

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QVariant>

#include <ranges>

#include "glog/logging.h"

#include "App/Utils/DirectionUtils.h"

namespace {

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
	Impl(QGeoPositionInfoSource * positionSource)
		: networkManager(new QNetworkAccessManager())
		, positionSource(positionSource)
	{
	}

	QNetworkAccessManager * networkManager;
	Items items;
	std::unordered_set<int> seenCids;
	QGeoPositionInfoSource * positionSource;
};

PastVuModel::PastVuModel(QGeoPositionInfoSource * positionSource, QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>(positionSource))
{
	connect(m_impl->positionSource, &QGeoPositionInfoSource::positionUpdated, this, [&](const QGeoPositionInfo & info) {
		const auto currentCoordinates = info.coordinate();
		const auto lat = currentCoordinates.latitude();
		const auto lon = currentCoordinates.longitude();
		const auto url = QString(R"(https://pastvu.com/api2?method=photo.giveNearestPhotos&params={"geo":[%1,%2],"limit":12,"except":228481})").arg(lat).arg(lon);
		QNetworkRequest request(url);
		m_impl->networkManager->get(request);
	});
	connect(m_impl->networkManager, &QNetworkAccessManager::finished, this, [&](QNetworkReply * reply) {
		LOG(INFO) << "Reply received";
		if (reply->error())
		{
			LOG(INFO) << "Reply error:" << reply->errorString().toStdString();
		}
		else
		{
			LOG(INFO) << "Reply success";
			const auto response = reply->readAll();

			QJsonParseError parserError;
			const auto jsonDoc = QJsonDocument::fromJson(response, &parserError);
			if (parserError.error != QJsonParseError::NoError)
				LOG(WARNING) << "Failed to parse JSON with error" << parserError.errorString().toStdString();

			const auto root = jsonDoc.object();
			const auto result = root.value("result").toObject();
			const auto photos = result.value("photos").toArray();

			auto newItemsView = photos
							  | std::views::transform([](const QJsonValue & v) { return v.toObject(); })
							  | std::views::filter([&](const QJsonObject & obj) {
									auto cid = obj.value("cid").toInt();
									if (m_impl->seenCids.contains(cid))
										return false;
									m_impl->seenCids.insert(cid);
									return true;
								})
							  | std::views::transform([&](const QJsonObject & obj) -> Item {
									const auto geo = obj.value("geo").toArray();
									return {
										obj.value("cid").toInt(),
										{ geo.at(0).toDouble(), geo.at(1).toDouble() },
										obj.value("file").toString(),
										obj.value("title").toString(),
										DirectionUtils::BearingFromDirection(obj.value("dir").toString()),
										obj.value("year").toInt()
									};
								});

			const auto newItems = Items(newItemsView.begin(), newItemsView.end());
			if (!newItems.empty())
			{
				beginInsertRows(QModelIndex(), m_impl->items.size(), m_impl->items.size() + newItems.size() - 1);
				m_impl->items.insert(m_impl->items.end(), newItems.begin(), newItems.end());
				endInsertRows();
			}
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

void PastVuModel::OnPositionPermissionGranted()
{
	if (m_impl->positionSource)
		m_impl->positionSource->startUpdates();
}
