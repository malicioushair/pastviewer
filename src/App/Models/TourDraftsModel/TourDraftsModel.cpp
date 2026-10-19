#include "TourDraftsModel.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <vector>

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
#include <QQmlEngine>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#include "App/Tours/Tours.h"
#include "App/Utils/JsonHelpers.h"

#include "PositionSourceAdapter.h"
#include "glog/logging.h"

namespace {

int64_t NextTourId(const std::vector<Tours::Tour> & tours)
{
    int64_t newTourID = 0;
    for (const auto & tour : tours)
        newTourID = std::max(newTourID, tour.id);
    return newTourID + 1;
}

void DeleteTourFile(const Tours::Tour & tour)
{
    const auto filePath = Tours::GetDraftFileLocation(tour.title);
    if (QFile file(filePath); !file.remove())
        LOG(WARNING) << std::format("Could not remove the file: {} with error: {}",
            filePath.toStdString(),
            file.errorString().toStdString());
}

double StraightPathDistance(const QList<QGeoCoordinate> & path)
{
    auto distance = 0.0;
    for (auto i = 1; i < path.size(); ++i)
        distance += path.at(i - 1).distanceTo(path.at(i));
    return distance;
}
}
struct TourDraftsModel::Impl
{
    const PositionSourceAdapter & positionSource;
    QNetworkAccessManager networkManager {};
    QTimer timer {};
    std::vector<Tours::Tour> tours {};
    int64_t editedTourId { -1 };

    int CurrentRow() const
    {
        if (tours.empty())
            return -1;

        for (int row = 0; row < static_cast<int>(tours.size()); ++row)
            if (tours[row].id == editedTourId)
                return row;
        return -1;
    }

    Tours::Tour * CurrentTour()
    {
        const auto row = CurrentRow();
        return row < 0 ? nullptr : &tours[row];
    }
};

TourDraftsModel::TourDraftsModel(const PositionSourceAdapter & positionSource, QObject * parent)
    : QAbstractListModel(parent)
    , m_impl(std::make_unique<Impl>(positionSource))
{
	Update();

    m_impl->timer.setInterval(5000);
    connect(&m_impl->timer, &QTimer::timeout, this, [&] {
        auto * tour = m_impl->CurrentTour();
        if (!tour || tour->tourPath.isEmpty())
            return;
        auto & currentTour = *tour;
        const auto lastAddedPos = currentTour.tourPath.last();
        const auto currentPos = m_impl->positionSource.Coordinate();
        currentTour.distance += lastAddedPos.distanceTo(currentPos);
        emit CurrentDistanceChanged();

        static constexpr auto MIN_DISTANCE = 10;
        if (currentPos.distanceTo(currentTour.tourPath.last()) < MIN_DISTANCE)
            return;

        currentTour.tourPath.emplace_back(currentPos);
        emit CurrentTourPathChanged();
    });
    connect(this, &TourDraftsModel::CurrentTourPathChanged, this, [&] {
        if (const auto * tour = m_impl->CurrentTour())
            Tours::SaveDraft(*tour);
    });
}

TourDraftsModel::~TourDraftsModel() = default;

int TourDraftsModel::rowCount(const QModelIndex &) const
{
	return static_cast<int>(m_impl->tours.size());
}

QVariant TourDraftsModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};
	const auto item = m_impl->tours.at(index.row());
	switch (role)
	{
		case Id:
            return static_cast<qlonglong>(item.id);
        case Title:
            return item.title;
        case Description:
			return item.description;
        case Distance:
            return item.distance;
        default:
            assert(false && "Unknown role");
    }
    return {};
}

bool TourDraftsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (!index.isValid())
		return assert(false && "invalid index"), false;

	auto item = m_impl->tours.at(index.row());
	switch (role)
	{
		case AddStop:
		{
			m_impl->tours.at(index.row()).stops.emplace_back(value.value<Tours::TourStop>());
			emit dataChanged(index, index);
			return true;
        }
        case Publish:
        {
            QUrl url("https://www.pastviewer.com/api/v1/admin/tours");
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            request.setRawHeader("X-Admin-Token", ADMIN_TOKEN);
            const auto payload = QJsonDocument(Tours::TourToJson(item, /*forPublish=*/true))
                                     .toJson(QJsonDocument::Compact);

            auto * reply = m_impl->networkManager.post(request, payload);
            LOG(INFO) << payload.toStdString();
            connect(reply, &QNetworkReply::finished, this, [this, item, reply, index] {
                const auto body = reply->readAll();
                LOG(INFO) << "Published tour '" << item.title.toStdString()
                          << "': " << body.toStdString();
                DeleteTourFile(item);
                DeleteTourItem(index);
            });
            return true;
        }
        case Delete:
        {
            DeleteTourFile(item);
            DeleteTourItem(index);
            return true;
        }
        default:
            assert(false && "unknown role");
    }
    return false;
}

QHash<int, QByteArray> TourDraftsModel::roleNames() const
{
#define ROLENAME(NAME)     \
	{                      \
		Roles::NAME, #NAME \
	}
    return {
        ROLENAME(Id),
        ROLENAME(Title),
        ROLENAME(Description),
        ROLENAME(Delete),
        ROLENAME(Distance),
        ROLENAME(Publish),
        ROLENAME(AddStop),
    };
#undef ROLENAME
}

void TourDraftsModel::Update()
{
	QJsonDocument doc;
	const auto dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/draft_tours";
	for (const auto & dirEntry : QDirListing(dirPath))
	{
		const auto json = JsonHelpers::ReadJson(dirEntry.absoluteFilePath());
        Tours::Tour tour {
#define PROPERTY(NAME) .NAME = json[#NAME]
            PROPERTY(id).toInt(),
            PROPERTY(title).toString(),
            PROPERTY(description).toString(),
            PROPERTY(distance).toDouble(),
            PROPERTY(localImagePath).toString(),
            PROPERTY(imageFileHash).toString(),
            PROPERTY(lang).toString(),
#undef PROPERTY
        };
        for (const auto & point : json["path"].toArray())
        {
            const auto parts = point.toString().split(',');
            tour.tourPath.append(QGeoCoordinate(parts.value(0).toDouble(), parts.value(1).toDouble()));
        }
        const auto stops = json["stops"].toArray();
        std::ranges::transform(stops, std::back_inserter(tour.stops), [](decltype(stops)::const_reference value) {
			const auto parts = value["coords"].toString().split(',');
			return Tours::TourStop {
#define PROPERTY(NAME) .NAME = value[#NAME]
				PROPERTY(id).toInt(),
				PROPERTY(title).toString(),
				PROPERTY(description).toString(),
				PROPERTY(localImagePath).toString(),
				PROPERTY(imageFileHash).toString(),
				PROPERTY(localAudioPath).toString(),
				PROPERTY(audioFileHash).toString(),
#undef PROPERTY
				.coords = QGeoCoordinate(parts.value(0).toDouble(), parts.value(1).toDouble()),
			};
		});
		if (std::ranges::none_of(m_impl->tours, [&](const Tours::Tour & item) { return item.id == tour.id; }))
		{
			beginInsertRows({}, rowCount(), rowCount());
			m_impl->tours.push_back(tour);
            endInsertRows();
        }
    }
}

std::vector<Tours::Tour> TourDraftsModel::GetTourDrafts() const
{
	return m_impl->tours;
}

void TourDraftsModel::AddTour(const Tours::Tour & tour)
{
	beginInsertRows({}, rowCount(), rowCount());
	m_impl->tours.emplace_back(tour);
    m_impl->editedTourId = tour.id;
    endInsertRows();
}

QVariantList TourDraftsModel::GetPlacedStopMarkers() const
{
    QVariantList markers;
    const auto * tour = m_impl->CurrentTour();
    if (!tour)
        return markers;
    for (const auto & stop : tour->stops)
        markers.append(QVariantMap {
            { "id",        static_cast<qlonglong>(stop.id)      },
            { "latitude",  stop.coords.latitude()               },
            { "longitude", stop.coords.longitude()              },
            { "number",    static_cast<int>(markers.size() + 1) }
        });
    return markers;
}

QVariantMap TourDraftsModel::GetDraft(qlonglong id) const
{
    const auto it = std::ranges::find(m_impl->tours, id, &Tours::Tour::id);
    if (it == m_impl->tours.end())
        return {};
    return {
        { "id",          id                 },
        { "title",       it->title          },
        { "description", it->description    },
        { "imageFile",   it->localImagePath }
    };
}

QString TourDraftsModel::UpdateDraftDetails(qlonglong id, const QString & title, const QString & description, const QUrl & imageFile)
{
    const auto it = std::ranges::find(m_impl->tours, id, &Tours::Tour::id);
    if (it == m_impl->tours.end())
        return tr("This draft no longer exists.");
    const auto cleanTitle = title.trimmed();
    if (cleanTitle.isEmpty() || cleanTitle.contains('/') || cleanTitle.contains('\\'))
        return tr("Enter a title without slashes.");
    auto updated = *it;
    updated.title = cleanTitle;
    updated.description = description;
    if (imageFile != updated.localImagePath)
    {
        const auto hash = Tours::ChosenFileContentHash(imageFile);
        if (!hash)
            return tr("Cannot read the selected cover image.");
        updated.localImagePath = imageFile;
        updated.imageFileHash = *hash;
    }
    const auto oldPath = Tours::GetDraftFileLocation(it->title);
    const auto newPath = Tours::GetDraftFileLocation(updated.title);
    if (oldPath != newPath && QFile::exists(newPath))
        return tr("A draft with this title already exists.");
    if (!Tours::SaveDraft(updated))
        return tr("Could not save the draft.");
    if (oldPath != newPath && QFile::exists(oldPath) && !QFile::remove(oldPath))
    {
        QFile::remove(newPath);
        return tr("Could not rename the draft file.");
    }
    *it = std::move(updated);
    const auto row = static_cast<int>(std::distance(m_impl->tours.begin(), it));
    emit dataChanged(index(row), index(row));
    return {};
}

bool TourDraftsModel::SelectDraft(qlonglong id)
{
    if (std::ranges::find(m_impl->tours, id, &Tours::Tour::id) == m_impl->tours.end())
        return false;
    StopRecording();
    m_impl->editedTourId = id;
    emit StopsUpdated();
    emit CurrentTourPathChanged();
    emit CurrentDistanceChanged();
    return true;
}

QVariantMap TourDraftsModel::GetDraftStop(qlonglong id) const
{
    const auto * tour = m_impl->CurrentTour();
    if (!tour)
        return {};
    const auto it = std::ranges::find(tour->stops, id, &Tours::TourStop::id);
    if (it == tour->stops.end())
        return {};
    return {
        { "id",          id                 },
        { "title",       it->title          },
        { "description", it->description    },
        { "imageFile",   it->localImagePath },
        { "audioFile",   it->localAudioPath }
    };
}

QString TourDraftsModel::UpdateDraftStop(qlonglong id, const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile)
{
    auto * tour = m_impl->CurrentTour();
    if (!tour)
        return tr("This draft no longer exists.");
    auto updated = *tour;
    auto it = std::ranges::find(updated.stops, id, &Tours::TourStop::id);
    if (it == updated.stops.end())
        return tr("This stop no longer exists.");
    if (title.trimmed().isEmpty())
        return tr("Enter a stop title.");
    if (it->localImagePath != imageFile)
    {
        const auto hash = Tours::ChosenFileContentHash(imageFile);
        if (!hash)
            return tr("Cannot read the selected image.");
        it->localImagePath = imageFile;
        it->imageFileHash = *hash;
    }
    if (it->localAudioPath != audioFile)
    {
        const auto hash = Tours::ChosenFileContentHash(audioFile);
        if (!hash)
            return tr("Cannot read the selected audio.");
        it->localAudioPath = audioFile;
        it->audioFileHash = *hash;
    }
    it->title = title.trimmed();
    it->description = description;
    if (!Tours::SaveDraft(updated))
        return tr("Could not save the stop.");
    *tour = std::move(updated);
    emit StopsUpdated();
    emit dataChanged(index(m_impl->CurrentRow()), index(m_impl->CurrentRow()));
    return {};
}

void TourDraftsModel::UpdateTourPath(const QGeoCoordinate & coords)
{
    auto * tour = m_impl->CurrentTour();
    if (!tour)
        return;
    auto & currentTour = *tour;
    currentTour.tourPath.emplace_back(coords);
}

void TourDraftsModel::AddPoint(const QGeoCoordinate & coordinate)
{
    auto * tour = m_impl->CurrentTour();
    if (!tour)
        return;
    auto & currentTour = *tour;
    currentTour.tourPath.emplace_back(coordinate);
    currentTour.distance = StraightPathDistance(currentTour.tourPath);
    EmitCurrentTourPathEdited();
}

void TourDraftsModel::RemoveLastPoint()
{
    auto * tour = m_impl->CurrentTour();
    if (!tour)
        return;
    auto & currentTour = *tour;
    if (currentTour.tourPath.isEmpty())
    {
        if (currentTour.distance != 0.0)
        {
            currentTour.distance = 0;
            emit CurrentDistanceChanged();
        }
        return;
    }

    const auto removed = currentTour.tourPath.takeLast();
    auto removedStop = false;
    for (auto i = currentTour.stops.size() - 1; i >= 0; --i)
    {
        if (currentTour.stops.at(i).coords != removed)
            continue;

        currentTour.stops.removeAt(i);
        removedStop = true;
    }

    currentTour.distance = StraightPathDistance(currentTour.tourPath);
    EmitCurrentTourPathEdited();
    if (removedStop)
        emit StopsUpdated();
}

void TourDraftsModel::EmitCurrentTourPathEdited()
{
    const auto row = m_impl->CurrentRow();
    if (row < 0)
        return;
    const auto modelIndex = index(row);
    emit dataChanged(modelIndex, modelIndex);
    emit CurrentDistanceChanged();
    emit CurrentTourPathChanged();
}

QList<QGeoCoordinate> TourDraftsModel::GetTourPath() const
{
    const auto * tour = m_impl->CurrentTour();
    return tour ? tour->tourPath : QList<QGeoCoordinate> {};
}

QList<QGeoCoordinate> TourDraftsModel::GetCurrentStopCoordinates() const
{
    const auto * tour = m_impl->CurrentTour();
    if (!tour)
        return {};
    const auto & currentTour = *tour;
    QList<QGeoCoordinate> coordinates;
    coordinates.reserve(currentTour.stops.size());
    for (const auto & stop : currentTour.stops)
        coordinates.append(stop.coords);
    return coordinates;
}

void TourDraftsModel::CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile, const QString & tourLang)
{
    const auto imageFileHash = Tours::ChosenFileContentHash(imageFile);
    if (!imageFileHash)
    {
        LOG(ERROR) << std::format("Cannot read file: {}", imageFile.path().toStdString());
        return;
    }

    Tours::Tour newTour = {
        .id = NextTourId(m_impl->tours),
        .title = title,
        .description = description,
        .distance = {},
        .localImagePath = imageFile,
        .imageFileHash = imageFileHash.value(),
        .tourPath = {},
        .stops = {},
        .lang = tourLang,
    };

    if (!Tours::SaveDraft(newTour))
        return;

    StopRecording();
    AddTour(newTour);
    emit StopsUpdated();
    emit CurrentTourPathChanged();
    emit CurrentDistanceChanged();
}

void TourDraftsModel::CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile, const QGeoCoordinate & coordinate)
{
    auto * tour = m_impl->CurrentTour();
    if (!tour)
        return;
    auto & currentTour = *tour;
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

    const auto newStopId = currentTour.stops.isEmpty() ? 0 : currentTour.stops.last().id + 1;
    const Tours::TourStop tourStop {
        .id = newStopId,
        .title = title,
        .description = description,
        .localImagePath = imageFile,
        .imageFileHash = imageFileHash.value(),
        .localAudioPath = audioFile,
        .audioFileHash = audioFileHash.value(),
        .coords = coordinate.isValid() ? coordinate : m_impl->positionSource.Coordinate()
    };
    currentTour.stops.emplace_back(tourStop);

    auto appendedToPath { false };
    if (coordinate.isValid())
    {
        const auto isSameAsLast = !currentTour.tourPath.isEmpty() && currentTour.tourPath.last() == coordinate;
        if (!isSameAsLast)
        {
            currentTour.tourPath.emplace_back(coordinate);
            currentTour.distance = StraightPathDistance(currentTour.tourPath);
            appendedToPath = true;
        }
    }

    if (!SaveDraft(currentTour))
        return;

    emit StopsUpdated();
    if (appendedToPath)
        EmitCurrentTourPathEdited();
}

QString TourDraftsModel::GetCurrentStops() const
{
    const auto * tour = m_impl->CurrentTour();
    return QString::number(tour ? tour->stops.size() : 0);
}

void TourDraftsModel::StartRecording()
{
    auto * tour = m_impl->CurrentTour();
    if (!tour)
        return;
    auto & currentTour = *tour;
    currentTour.distance = 0;
    emit CurrentDistanceChanged();
    currentTour.tourPath.clear();
    currentTour.tourPath.emplace_back(m_impl->positionSource.Coordinate());
    emit CurrentTourPathChanged();
    m_impl->timer.start();
}

void TourDraftsModel::PauseRecording()
{
    m_impl->timer.stop();
}

void TourDraftsModel::ResumeRecording()
{
    m_impl->timer.start();
}

void TourDraftsModel::StopRecording()
{
    m_impl->timer.stop();
}

QString TourDraftsModel::GetDistance()
{
    const auto * tour = m_impl->CurrentTour();
    return TourDraftsModel::tr("%1 %2").arg(QString::number(tour ? tour->distance : 0, 'f', 0), "m");
}

void TourDraftsModel::DeleteTourItem(const QModelIndex & index)
{
    if (false
        || !index.isValid()
        || index.row() >= m_impl->tours.size())
        return;

    const bool wasCurrent = m_impl->CurrentRow() == index.row();
    if (wasCurrent)
        StopRecording();
    beginRemoveRows({}, index.row(), index.row());
    m_impl->tours.erase(m_impl->tours.cbegin() + index.row());
    endRemoveRows();
    if (wasCurrent)
    {
        m_impl->editedTourId = -1;
        emit StopsUpdated();
        emit CurrentTourPathChanged();
        emit CurrentDistanceChanged();
    }
}

void TourDraftsModel::UploadAsset(const QUrl & assetFile)
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

        emit AssetUploadFinished();
        reply->deleteLater();
    });
}
