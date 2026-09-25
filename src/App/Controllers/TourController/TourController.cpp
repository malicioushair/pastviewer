#include "TourController.h"

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QGeoCoordinate>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Models/BaseModel.h"
#include "App/Models/TourDraftsModel/TourDraftsModel.h"
#include "App/Tours/Tours.h"
#include "App/Utils/JsonHelpers.h"

struct TourController::Impl
{
	const PositionSourceAdapter & positionSource;
	QList<QGeoCoordinate> tourPath {};
	QList<Tours::Tour> availableTours {};
	double distance {};
	QTimer timer {};
	TourDraftsModel tourDraftsModel {};
	QNetworkAccessManager networkManager {};
};

TourController::TourController(const PositionSourceAdapter & positionSource, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(positionSource))
{
	m_impl->timer.setInterval(5000);
	connect(&m_impl->timer, &QTimer::timeout, this, [&] {
		const auto lastAddedPos = m_impl->tourPath.last();
		const auto currentPos = m_impl->positionSource.Coordinate();
		m_impl->distance += lastAddedPos.distanceTo(currentPos);
		emit DistanceChanged();

		m_impl->tourPath.emplace_back(currentPos);
		emit TourPathChanged();
	});
	connect(&m_impl->networkManager, &QNetworkAccessManager::finished, this, &TourController::OnNetworkReplyFinished);
}

TourController::~TourController() = default;

void TourController::StartRecording()
{
	m_impl->distance = 0;
	emit DistanceChanged();

	m_impl->tourPath.clear();
	m_impl->tourPath.emplace_back(m_impl->positionSource.Coordinate());
	emit TourPathChanged();

	m_impl->timer.start();
}

void TourController::PauseRecording()
{
	m_impl->timer.stop();
}

void TourController::UnpauseRecording()
{
	m_impl->timer.start();
}

void TourController::StopRecording()
{
	m_impl->timer.stop();
}

void TourController::CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile)
{
	m_impl->tourDraftsModel.AddTour(Tours::Tour {
		.id = NextTourId(),
		.title = title,
		.description = description,
		.imageFile = imageFile,
		.stops = {},
	});

	SaveDraft(m_impl->tourDraftsModel.GetTourDrafts().back());
}

void TourController::CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile)
{
	auto currentTour = m_impl->tourDraftsModel.rowCount() == 0 ? Tours::Tour {} : m_impl->tourDraftsModel.GetTourDrafts().back();
	const auto newStopId = currentTour.stops.isEmpty() ? 0 : currentTour.stops.last().id + 1;
	const Tours::TourStop tourStop {
		.id = newStopId,
		.title = title,
		.description = description,
		.imageFile = imageFile,
		.audioFile = audioFile,
		.coords = m_impl->positionSource.Coordinate()
	};
	currentTour.stops.emplace_back(tourStop);

	SaveDraft(currentTour);
	m_impl->tourDraftsModel.Update();
}

QAbstractListModel * TourController::GetDraftsModel() const
{
	return &m_impl->tourDraftsModel;
}

void TourController::UpdateModel()
{
	m_impl->tourDraftsModel.Update();
}

QString TourController::GetDistance() const
{
	return TourController::tr("%1 %2").arg(QString::number(m_impl->distance, 'f', 0), "m");
}

void TourController::SaveDraft(const Tours::Tour & tour)
{
	QJsonObject tourObj;
	tourObj["schemaVersion"] = 1;
	tourObj["id"] = static_cast<qint64>(tour.id);
	tourObj["title"] = tour.title;
	tourObj["description"] = tour.description;
	tourObj["imageFile"] = tour.imageFile.toString();

	QJsonArray stops;
	std::ranges::transform(
		tour.stops,
		std::back_inserter(stops),
		[](decltype(tour.stops)::const_reference stop) {
			QJsonObject stopObj;
			stopObj["id"] = static_cast<qint64>(stop.id);
			stopObj["title"] = stop.title;
			stopObj["description"] = stop.description;
			stopObj["imageFile"] = stop.imageFile.toString();
			stopObj["audioFile"] = stop.audioFile.toString();
			stopObj["coords"] = QString("%1, %2").arg(stop.coords.latitude()).arg(stop.coords.longitude());
			return stopObj;
		});

	tourObj["stops"] = stops;

	QJsonDocument doc(tourObj);
	JsonHelpers::SaveJson(doc, tour.title);
}

void TourController::OnNetworkReplyFinished(QNetworkReply * reply)
{
	const auto row = reply->property("row").toInt();
	const auto tourTitle = reply->property("tourTitle").toString();
	if (reply->error() != QNetworkReply::NoError)
	{
		emit TourPublished(row, false, reply->errorString());
		reply->deleteLater();
		return;
	}
	const auto body = reply->readAll();
	reply->deleteLater();
	QJsonParseError parseError;
	if (parseError.error != QJsonParseError::NoError)
	{
		emit TourPublished(row, false, parseError.errorString());
		return;
	}
	LOG(INFO) << "Published tour '" << tourTitle.toStdString()
			  << "': " << body.toStdString();
	m_impl->tourDraftsModel.setData(m_impl->tourDraftsModel.index(row), true, TourDraftsModel::Roles::Delete);
	emit TourPublished(row, true, {});
}

QList<QGeoCoordinate> TourController::GetTourPath() const
{
	return m_impl->tourPath;
}

int64_t TourController::NextTourId() const
{
	auto newTourID = m_impl->availableTours.isEmpty() ? 0 : m_impl->availableTours.last().id;
	for (const auto & tour : m_impl->tourDraftsModel.GetTourDrafts())
		newTourID = std::max(newTourID, tour.id);
	return newTourID + 1;
}

void TourController::PublishTour(int row)
{
	const auto drafts = m_impl->tourDraftsModel.GetTourDrafts();
	const auto & tour = drafts.at(row);
	QUrl url("https://www.pastviewer.com/api/v1/admin/tours");
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("X-Admin-Token", ADMIN_TOKEN);
	const auto payload = QJsonDocument(Tours::TourToJson(tour, /*forPublish=*/true))
							 .toJson(QJsonDocument::Compact);
	auto * reply = m_impl->networkManager.post(request, payload);
	reply->setProperty("row", row);
	reply->setProperty("tourTitle", tour.title);
}
