#include "TourController.h"

#include <algorithm>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QGeoCoordinate>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <vector>

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
	const auto imageFileHash = Tours::ChosenFileContentHash(imageFile);
	if (!imageFileHash)
	{
		LOG(ERROR) << std::format("Cannot read file: {}", imageFile.path().toStdString());
		return;
	}

	Tours::Tour tour {
		.id = NextTourId(),
		.title = title,
		.description = description,
		.localImagePath = imageFile,
		.imageFileHash = imageFileHash.value(),
		.stops = {},
	};

	if (!SaveDraft(tour))
		return;

	m_impl->tourDraftsModel.AddTour(tour);
}

void TourController::CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile)
{
	auto currentTour = m_impl->tourDraftsModel.rowCount() == 0 ? Tours::Tour {} : m_impl->tourDraftsModel.GetTourDrafts().back();
	const auto newStopId = currentTour.stops.isEmpty() ? 0 : currentTour.stops.last().id + 1;
	const auto imageFileHash = Tours::ChosenFileContentHash(imageFile);
	const auto audioFileHash = Tours::ChosenFileContentHash(audioFile);
	if (!imageFileHash)
	{
		LOG(ERROR) << std::format("Cannot read file: {}", imageFile.path().toStdString());
		return;
	}
	if (!audioFileHash)
	{
		LOG(ERROR) << std::format("Cannot read file: {}", audioFile.path().toStdString());
		return;
	}

	const Tours::TourStop tourStop {
		.id = newStopId,
		.title = title,
		.description = description,
		.localImagePath = imageFile,
		.imageFileHash = imageFileHash.value(),
		.localAudioPath = audioFile,
		.audioFileHash = audioFileHash.value(),
		.coords = m_impl->positionSource.Coordinate()
	};
	currentTour.stops.emplace_back(tourStop);

	if (!SaveDraft(currentTour))
		return;

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

bool TourController::SaveDraft(const Tours::Tour & tour)
{
	QJsonArray stops;
	for (auto & stop : tour.stops)
	{
		QJsonObject stopObj;
		stopObj["id"] = static_cast<qint64>(stop.id);
		stopObj["title"] = stop.title;
		stopObj["description"] = stop.description;
		stopObj["localImagePath"] = stop.localImagePath.toString();
		stopObj["imageFileHash"] = stop.imageFileHash;
		stopObj["localAudioPath"] = stop.localAudioPath.toString();
		stopObj["audioFileHash"] = stop.audioFileHash;
		stopObj["coords"] = QString("%1, %2").arg(stop.coords.latitude()).arg(stop.coords.longitude());
		stops.append(stopObj);
	}

	QJsonObject tourObj;
	tourObj["schemaVersion"] = 1;
	tourObj["id"] = static_cast<qint64>(tour.id);
	tourObj["title"] = tour.title;
	tourObj["description"] = tour.description;
	tourObj["localImagePath"] = tour.localImagePath.toString();
	tourObj["imageFileHash"] = tour.imageFileHash;
	tourObj["stops"] = stops;

	QJsonDocument doc(tourObj);
	return JsonHelpers::SaveJson(doc, tour.title);
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
	const auto tour = drafts.at(row);

	QUrl url("https://www.pastviewer.com/api/v1/admin/tours");
	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
	request.setRawHeader("X-Admin-Token", ADMIN_TOKEN);
	const auto payload = QJsonDocument(Tours::TourToJson(tour, /*forPublish=*/true))
							 .toJson(QJsonDocument::Compact);

	auto * reply = m_impl->networkManager.post(request, payload);
	connect(reply, &QNetworkReply::finished, this, [this, row, tour, reply] {
		const auto body = reply->readAll();
		LOG(INFO) << "Published tour '" << tour.title.toStdString()
				  << "': " << body.toStdString();
		m_impl->tourDraftsModel.setData(m_impl->tourDraftsModel.index(row), true, TourDraftsModel::Roles::Delete);
	});
}

void TourController::UploadAsset(const QUrl & assetFile)
{
	auto multipart = std::make_unique<QHttpMultiPart>(QHttpMultiPart::FormDataType);
	auto * file = new QFile(assetFile.toLocalFile(), multipart.get());
	if (file->fileName().isEmpty())
		return;

	if (!file->open(QIODevice::ReadOnly))
	{
		emit ImageUploadFailed(file->errorString());
		return;
	}

	const auto mimeType = QMimeDatabase().mimeTypeForFile(file->fileName(), QMimeDatabase::MatchContent).name();
	if (!mimeType.startsWith("image/") && !mimeType.startsWith("audio/"))
	{
		emit ImageUploadFailed(tr("The selected file is not an image or audio."));
		return;
	}

	const auto assetType = mimeType.startsWith("image/") ? "image"
						 : mimeType.startsWith("audio/")
							 ? "audio"
							 : "";
	QHttpPart part;
	part.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"file\"; filename=\"%1\"").arg(assetFile.fileName()));
	part.setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
	part.setBodyDevice(file);
	multipart->append(part);

	if (const auto imageFileHash = Tours::ChosenFileContentHash(assetFile))
	{
		QHttpPart hashPart;
		hashPart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"%1FileHash\"").arg(assetType));
		hashPart.setBody(imageFileHash->toUtf8());
		multipart->append(hashPart);
	}

	QNetworkRequest request(QUrl("https://www.pastviewer.com/api/v1/admin/assets"));
	request.setRawHeader("X-Admin-Token", ADMIN_TOKEN);
	request.setRawHeader("X-Asset-Kind", assetType);
	auto * reply = m_impl->networkManager.post(request, multipart.get());
	multipart.release()->setParent(reply);
	connect(reply, &QNetworkReply::finished, this, [&, reply] {
		if (reply->error() != QNetworkReply::NoError)
		{
			LOG(ERROR) << reply->errorString().toStdString();
			emit ImageUploadFailed(reply->errorString());
			return;
		}

		QJsonParseError parseError;
		const auto document = QJsonDocument::fromJson(reply->readAll(), &parseError);
		if (false
			|| reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 201
			|| parseError.error != QJsonParseError::NoError || !document.isObject())
		{
			emit ImageUploadFailed(tr("The server returned an invalid image upload response."));
			return;
		}

		emit assetUploadFinished();
		reply->deleteLater();
	});
}
