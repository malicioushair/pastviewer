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

#include <memory>
#include <ranges>
#include <vector>

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
	Items items { &Item::cid };
	QGeoPositionInfoSource * positionSource;
	QUrl url { "https://pastvu.com/api2" };
	int zoomLevel;
	quint64 requestNumber { 0 };
	QGeoRectangle lastKnownViewport {};
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

		emit LoadingItems();
		m_impl->lastKnownViewport = viewport;
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
		auto * reply = m_impl->networkManager->get(request);
		reply->setProperty("requestNumber", ++m_impl->requestNumber);
	});

	connect(m_impl->networkManager.get(), &QNetworkAccessManager::finished, this, &BaseModel::OnNetworkReplyFinished);
}

BaseModel::~BaseModel() = default;

int BaseModel::rowCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		return 0;
	return static_cast<int>(m_impl->items.Size());
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

	if (index.row() < 0 || index.row() >= static_cast<int>(m_impl->items.Size()))
		return assert(false && "Invalid index"), QVariant();

	static constexpr auto fullSizeImageUrl = "https://pastvu.com/_p/a/";
	static constexpr auto thumbnailUrl = "https://pastvu.com/_p/h/";
	const auto item = m_impl->items.At(index.row());
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

	if (index.row() < 0 || index.row() >= static_cast<int>(m_impl->items.Size()))
		return assert(false && "Invalid index"), false;

	auto & item = m_impl->items.At(index.row());
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

void BaseModel::ReloadItems()
{
	m_impl->items.Clear();
	emit UpdateCoords(m_impl->lastKnownViewport);
}

void BaseModel::OnNetworkReplyFinished(QNetworkReply * reply)
{
	const auto requestNumber = reply->property("requestNumber").toInt();
	if (const auto skipNonLastReply = requestNumber != m_impl->requestNumber)
		return;

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
	const auto newItemsView = photos
							| std::views::transform([](const QJsonValue & v) { return v.toObject(); })
							| std::views::transform([](const QJsonObject & obj) { return JsonObjectToItem(obj); });

	AddItemsToModel(std::vector<Item>(newItemsView.begin(), newItemsView.end()));
}

void BaseModel::AddItemsToModel(std::span<const Item> newItems)
{
	if (newItems.empty())
		return;

	beginResetModel();
	m_impl->items.Push(newItems);
	endResetModel();
	emit ItemsLoaded();
}
