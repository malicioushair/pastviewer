#include "ScreenObjectsModel.h"

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QVariant>

#include <ranges>

#include "App/Models/BaseModel.h"
#include "glog/logging.h"

#include "App/Utils/DirectionUtils.h"

struct ScreenObjectsModel::Impl
{
	int zoomLevel { 13 };
};

ScreenObjectsModel::ScreenObjectsModel(QGeoPositionInfoSource * positionSource, QObject * parent)
	: BaseModel(positionSource, parent)
	, m_screenImpl(std::make_unique<Impl>())
{
	connect(this, &ScreenObjectsModel::UpdateCoords, this, [this](const QGeoRectangle & viewport) {
		if (m_screenImpl->zoomLevel < 9)
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
		GetMutableUrl().setQuery(query);

		QNetworkRequest request(GetUrl());
		GetNetworkManager()->get(request);
	});

	connect(GetNetworkManager(), &QNetworkAccessManager::finished, this, [this](QNetworkReply * reply) {
		LOG(INFO) << "Reply received";
		if (reply->error())
		{
			LOG(INFO) << "Reply error:" << reply->errorString().toStdString();
			return;
		}
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
						  | std::views::filter([this](const QJsonObject & obj) {
								auto cid = obj.value("cid").toInt();
								if (GetSeenCids().contains(cid))
									return false;
								GetMutableSeenCids().insert(cid);
								return true;
							})
						  | std::views::transform([this](const QJsonObject & obj) -> Item {
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
		static constexpr auto MAX_ITEMS_PER_MODEL = 300;
		if (!newItems.empty() && GetItems().size() > MAX_ITEMS_PER_MODEL)
		{
			if (newItems.size() > GetItems().size())
				GetMutableItems().clear();
			else
				GetMutableItems().erase(GetItems().cbegin(), GetItems().cbegin() + newItems.size());
		}
		if (!newItems.empty())
		{
			beginInsertRows(QModelIndex(), GetItems().size(), GetItems().size() + newItems.size() - 1);
			GetMutableItems().insert(GetItems().end(), newItems.begin(), newItems.end());
			endInsertRows();
		}

		reply->deleteLater();
	});
}

ScreenObjectsModel::~ScreenObjectsModel() = default;

int ScreenObjectsModel::rowCount(const QModelIndex & parent) const
{
	return GetItems().size();
}

QVariant ScreenObjectsModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
	{
		switch (role)
		{
			case Roles::ZoomLevel:
				return m_screenImpl->zoomLevel;
			default:
				assert(false && "Unexpected role");
		}
	}

	return BaseModel::data(index, role);
}

bool ScreenObjectsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (!index.isValid())
	{
		switch (role)
		{
			case Roles::ZoomLevel:
			{
				m_screenImpl->zoomLevel = value.toInt();
				return true;
			}
			default:
				assert(false && "Unexpected role");
		}
	}

	return BaseModel::setData(index, value, role);
}

QHash<int, QByteArray> ScreenObjectsModel::roleNames() const
{
	auto roles = BaseModel::roleNames();

#define ADD_ROLE(NAME) roles[NAME] = #NAME
	ADD_ROLE(ZoomLevel);
#undef ADD_ROLE
	return roles;
}

void ScreenObjectsModel::OnPositionPermissionGranted()
{
	if (GetPositionSource())
		GetPositionSource()->startUpdates();
}
