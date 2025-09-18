#include "PastVuModel.h"

#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkrequest>
#include <QObject>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qvariant.h>
#include <QtPositioning/qgeopositioninfo.h>
#include <QtPositioning/qgeopositioninfosource.h>
#include <QtQml/qqmlengine.h>
#include <memory>

#include "glog/logging.h"

struct Item
{
	int cid = 0;
	QGeoCoordinate coord;
	QString file;
	QString title;
	int year = 0;
};

enum Roles
{
	Coordinate = Qt::UserRole + 1,
	Title,
};

using Items = std::vector<Item>;

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
			qDebug() << "Position updated:" << info.coordinate();
			const auto currentCoordinates = info.coordinate();
			const auto lat = currentCoordinates.latitude();
			const auto lon = currentCoordinates.longitude();
			const auto url = QString(R"(https://pastvu.com/api2?method=photo.giveNearestPhotos&params={"geo":[%1,%2],"limit":12,"except":228481})").arg(lat).arg(lon);
			QNetworkRequest request(url);
			m_impl->networkManager->get(request);
		});
		source->startUpdates(); // Start receiving position updates
		qDebug() << "Position updates started.";
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
			qDebug() << "Response:" << response;

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

				beginResetModel();
				m_impl->items.push_back({
					jsonObj.value("cid").toInt(),
					{ geo.at(0).toDouble(), geo.at(1).toDouble() },
					jsonObj.value("file").toString(),
					jsonObj.value("title").toString(),
					jsonObj.value("year").toInt()
                });
				endResetModel();
				LOG(INFO) << m_impl->items.back().title.toStdString();
				auto foo = 0;
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
		default:
			assert(false && "Unexpected role");
	}

	return {};
}

QHash<int, QByteArray> PastVuModel::roleNames() const
{
	return {
		{ Roles::Coordinate, "coordinate" },
		{ Roles::Title,      "title"      },
	};
}
