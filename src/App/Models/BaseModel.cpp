#include "BaseModel.h"

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>

#include <cassert>
#include <memory>
#include <ranges>

#include "glog/logging.h"

#include "App/Utils/DirectionUtils.h"

namespace {

Item JsonObjectToItem(const QJsonObject & obj)
{
	const auto geo = obj.value("geo").toArray();
	return {
		obj.value("cid").toInt(),
		{ geo.at(0).toDouble(), geo.at(1).toDouble() },
		obj.value("file").toString(),
		obj.value("title").toString(),
		DirectionUtils::BearingFromDirection(obj.value("dir").toString()),
		obj.value("year").toInt()
	};
}
}

struct BaseModel::Impl
{
	explicit Impl(QGeoPositionInfoSource * positionSource)
		: networkManager(std::make_unique<QNetworkAccessManager>())
		, positionSource(positionSource)
		, zoomLevel(13)
	{
	}

	~Impl() = default;

	NON_COPY_MOVABLE(Impl);

	std::unique_ptr<QNetworkAccessManager> networkManager;
	Items items;
	std::unordered_set<int> seenCids;
	QGeoPositionInfoSource * positionSource;
	QUrl url { "https://pastvu.com/api2" };
	int zoomLevel;
};

BaseModel::BaseModel(QGeoPositionInfoSource * positionSource, QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>(positionSource))
{
	connect(this, &QAbstractListModel::rowsInserted, this, [this] { emit CountChanged(); });
	connect(this, &QAbstractListModel::rowsRemoved, this, [this] { emit CountChanged(); });
	connect(this, &QAbstractListModel::modelReset, this, [this] { emit CountChanged(); });

	connect(this, &BaseModel::UpdateCoords, this, [this](const QGeoRectangle & viewport) {
		if (m_impl->zoomLevel < 9)
			return;

		const auto paramsJson = QString(R"({"z":16,"geometry":{"type":"Polygon","coordinates":[[[%1,%2],[%3,%4],[%5,%6],[%7,%8],[%9,%10]]]},"localWork":0})")
									.arg(QString::number(viewport.topLeft().longitude(), 'f', 15))
									.arg(QString::number(viewport.topLeft().latitude(), 'f', 15))
									.arg(QString::number(viewport.bottomLeft().longitude(), 'f', 15))
									.arg(QString::number(viewport.bottomLeft().latitude(), 'f', 15))
									.arg(QString::number(viewport.bottomRight().longitude(), 'f', 15))
									.arg(QString::number(viewport.bottomRight().latitude(), 'f', 15))
									.arg(QString::number(viewport.topRight().longitude(), 'f', 15))
									.arg(QString::number(viewport.topRight().latitude(), 'f', 15))
									.arg(QString::number(viewport.topLeft().longitude(), 'f', 15))
									.arg(QString::number(viewport.topLeft().latitude(), 'f', 15));

		QUrlQuery query;
		query.addQueryItem("method", "photo.getByBounds");
		query.addQueryItem("params", paramsJson);
		m_impl->url.setQuery(query);

		QNetworkRequest request(m_impl->url);
		m_impl->networkManager->get(request);
	});

	connect(m_impl->networkManager.get(), &QNetworkAccessManager::finished, this, &BaseModel::OnNetworkReplyFinished);
}

BaseModel::~BaseModel() = default;

int BaseModel::rowCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		return 0;
	return static_cast<int>(m_impl->items.size());
}

QVariant BaseModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
	{
		switch (role)
		{
			case Roles::ZoomLevel:
				return m_impl->zoomLevel;
			default:
				assert(false && "Unexpected role");
		}
	}

	if (index.row() < 0 || index.row() >= static_cast<int>(m_impl->items.size()))
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

bool BaseModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (!index.isValid())
	{
		switch (role)
		{
			case Roles::ZoomLevel:
			{
				m_impl->zoomLevel = value.toInt();
				return true;
			}
			default:
				assert(false && "Unexpected role");
		}
		return false;
	}

	if (index.row() < 0 || index.row() >= static_cast<int>(m_impl->items.size()))
		return assert(false && "Invalid index"), false;

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

QHash<int, QByteArray> BaseModel::roleNames() const
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
		ROLENAME(ZoomLevel),
	};
#undef ROLENAME
}

void BaseModel::OnPositionPermissionGranted()
{
	if (m_impl->positionSource)
		m_impl->positionSource->startUpdates();
}

void BaseModel::OnNetworkReplyFinished(QNetworkReply * reply)
{
	LOG(INFO) << "Reply received";
	if (reply->error())
	{
		LOG(INFO) << "Reply error:" << reply->errorString().toStdString();
		reply->deleteLater();
		return;
	}

	LOG(INFO) << "Reply success";
	const auto response = reply->readAll();
	reply->deleteLater();

	QJsonParseError parserError;
	const auto jsonDoc = QJsonDocument::fromJson(response, &parserError);
	if (parserError.error != QJsonParseError::NoError)
	{
		LOG(WARNING) << "Failed to parse JSON with error" << parserError.errorString().toStdString();
		return;
	}

	const auto root = jsonDoc.object();
	const auto result = root.value("result").toObject();
	const auto photos = result.value("photos").toArray();

	if (!photos.isEmpty())
		ProcessPhotos(photos);
}

void BaseModel::ProcessPhotos(const QJsonArray & photos)
{
	auto newItemsView = photos
					  | std::views::transform([](const QJsonValue & v) { return v.toObject(); })
					  | std::views::filter([this](const QJsonObject & obj) {
							auto cid = obj.value("cid").toInt();
							if (m_impl->seenCids.contains(cid))
								return false;
							m_impl->seenCids.insert(cid);
							return true;
						})
					  | std::views::transform([](const QJsonObject & obj) { return JsonObjectToItem(obj); });

	const auto newItems = Items(newItemsView.begin(), newItemsView.end());
	AddItemsToModel(newItems);
}

void BaseModel::AddItemsToModel(const Items & newItems)
{
	if (newItems.empty())
		return;

	static constexpr auto MAX_ITEMS_PER_MODEL = 300;
	if (m_impl->items.size() > MAX_ITEMS_PER_MODEL)
	{
		if (newItems.size() > m_impl->items.size())
			m_impl->items.clear();
		else
			m_impl->items.erase(m_impl->items.cbegin(), m_impl->items.cbegin() + newItems.size());
	}

	beginInsertRows(QModelIndex(), m_impl->items.size(), m_impl->items.size() + newItems.size() - 1);
	m_impl->items.insert(m_impl->items.end(), newItems.begin(), newItems.end());
	endInsertRows();
}
