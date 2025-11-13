#include "NearestObjectsModel.h"

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QVariant>

#include <ranges>

#include "App/Models/BaseModel.h"
#include "glog/logging.h"

#include "App/Utils/DirectionUtils.h"

NearestObjectsModel::NearestObjectsModel(QGeoPositionInfoSource * _positionSource, QObject * parent)
	: BaseModel(_positionSource, parent)
{
	connect(GetPositionSource(), &QGeoPositionInfoSource::positionUpdated, this, [this](const QGeoPositionInfo & info) {
		const auto currentCoordinates = info.coordinate();
		const auto lat = currentCoordinates.latitude();
		const auto lon = currentCoordinates.longitude();
		const auto paramsJson = QString(R"({"geo":[%1,%2],"limit":12,"except":228481})").arg(lat).arg(lon);

		QUrlQuery query;
		query.addQueryItem("method", "photo.giveNearestPhotos");
		query.addQueryItem("params", paramsJson);
		GetMutableUrl().setQuery(query);

		QNetworkRequest request(GetMutableUrl());
		GetNetworkManager()->get(request);
	});
	connect(GetNetworkManager(), &QNetworkAccessManager::finished, this, [this](QNetworkReply * reply) {
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
							  | std::views::filter([this](const QJsonObject & obj) {
									auto cid = obj.value("cid").toInt();
									if (GetMutableSeenCids().contains(cid))
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
			if (!newItems.empty())
			{
				beginInsertRows(QModelIndex(), GetMutableItems().size(), GetMutableItems().size() + newItems.size() - 1);
				GetMutableItems().insert(GetMutableItems().end(), newItems.begin(), newItems.end());
				endInsertRows();
			}
		}
		reply->deleteLater();
	});
}

NearestObjectsModel::~NearestObjectsModel() = default;
