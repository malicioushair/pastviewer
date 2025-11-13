#include "BaseModel.h"

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QVariant>

#include <cassert>
#include <memory>

#include "App/Utils/NonCopyMovable.h"

struct BaseModel::Impl
{
	explicit Impl(QGeoPositionInfoSource * positionSource)
		: networkManager(std::make_unique<QNetworkAccessManager>())
		, positionSource(positionSource)
	{
	}

	~Impl() = default;

	NON_COPY_MOVABLE(Impl);

	std::unique_ptr<QNetworkAccessManager> networkManager;
	Items items;
	std::unordered_set<int> seenCids;
	QGeoPositionInfoSource * positionSource;
	QUrl url { "https://pastvu.com/api2" };
};

BaseModel::BaseModel(QGeoPositionInfoSource * positionSource, QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>(positionSource))
{
	connect(this, &QAbstractListModel::rowsInserted, this, [this] { emit CountChanged(); });
	connect(this, &QAbstractListModel::rowsRemoved, this, [this] { emit CountChanged(); });
	connect(this, &QAbstractListModel::modelReset, this, [this] { emit CountChanged(); });
}

BaseModel::~BaseModel() = default;

int BaseModel::rowCount(const QModelIndex & parent) const
{
	return GetItems().size();
}

QVariant BaseModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return assert(false && "Invalid index"), QVariant();

	static constexpr auto fullSizeImageUrl = "https://pastvu.com/_p/a/";
	static constexpr auto thumbnailUrl = "https://pastvu.com/_p/h/";
	const auto item = GetItems().at(index.row());
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
		return assert(false && "Invalid index"), false;

	auto & item = GetMutableItems().at(index.row());
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
	};
#undef ROLENAME
}

void BaseModel::OnPositionPermissionGranted()
{
	if (GetPositionSource())
		GetPositionSource()->startUpdates();
}

QNetworkAccessManager * BaseModel::GetNetworkManager() const
{
	return m_impl->networkManager.get();
}

Items & BaseModel::GetMutableItems()
{
	return m_impl->items;
}

const Items & BaseModel::GetItems() const
{
	return m_impl->items;
}

std::unordered_set<int> & BaseModel::GetMutableSeenCids()
{
	return m_impl->seenCids;
}

const std::unordered_set<int> & BaseModel::GetSeenCids() const
{
	return m_impl->seenCids;
}

QGeoPositionInfoSource * BaseModel::GetPositionSource() const
{
	return m_impl->positionSource;
}

QUrl & BaseModel::GetMutableUrl()
{
	return m_impl->url;
}

const QUrl & BaseModel::GetUrl() const
{
	return m_impl->url;
}
