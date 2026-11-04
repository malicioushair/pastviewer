#include "TourDraftsModel.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <vector>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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
#include <QQmlFile>
#include <QRegularExpression>
#include <QSaveFile>
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

QString RequestFailure(QNetworkReply * reply, const QByteArray & body)
{
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto error = QJsonDocument::fromJson(body).object().value("error").toObject();
    auto detail = error.value("message").toString();
    if (!error.value("code").toString().isEmpty())
        detail = error.value("code").toString() + ": " + detail;
    const auto details = error.value("details");
    if (details.isArray())
        detail += "\n" + QString::fromUtf8(QJsonDocument(details.toArray()).toJson(QJsonDocument::Compact));
    else if (details.isString())
        detail += "\n" + details.toString();
    if (detail.isEmpty())
        detail = body.isEmpty() ? reply->errorString() : QString::fromUtf8(body);
    return QObject::tr("HTTP %1: %2").arg(status).arg(detail);
}

double StraightPathDistance(const QList<QGeoCoordinate> & path)
{
    auto distance = 0.0;
    for (auto i = 1; i < path.size(); ++i)
        distance += path.at(i - 1).distanceTo(path.at(i));
    return distance;
}
}

struct TourDraftsModel::AssetUpload
{
    QList<QUrl> files;
    QHash<QUrl, QString> expectedHashes;
    qsizetype next { 0 };
    UploadCompletion completion;
};

struct TourDraftsModel::PublishOperation
{
    Tours::Tour tour;
    QByteArray payload;
};

struct TourDraftsModel::Impl
{
    const PositionSourceAdapter & positionSource;
    QNetworkAccessManager ownedNetworkManager {};
    QNetworkAccessManager & networkManager;
    QTimer timer {};
    std::vector<Tours::Tour> tours {};
    int64_t editedTourId { -1 };
    QHash<qlonglong, std::shared_ptr<PublishOperation>> publishing;

    Impl(const PositionSourceAdapter & position, QNetworkAccessManager * manager)
        : positionSource(position)
        , networkManager(manager ? *manager : ownedNetworkManager)
    {
    }

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

TourDraftsModel::TourDraftsModel(const PositionSourceAdapter & positionSource, QObject * parent, QNetworkAccessManager * networkManager)
    : QAbstractListModel(parent)
    , m_impl(std::make_unique<Impl>(positionSource, networkManager))
{
    Update();

    m_impl->timer.setInterval(5000);
    connect(&m_impl->timer, &QTimer::timeout, this, [&] {
        auto * tour = m_impl->CurrentTour();
        if (!tour || m_impl->publishing.contains(tour->id) || tour->tourPath.isEmpty())
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
        case Uploading:
            return m_impl->publishing.contains(item.id);
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
    if (m_impl->publishing.contains(item.id))
        return false;
    switch (role)
    {
        case AddStop:
        {
            m_impl->tours.at(index.row()).stops.emplace_back(value.value<Tours::TourStop>());
            emit dataChanged(index, index);
            return true;
        }
        case Publish:
            return StartPublishing(item);
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
        ROLENAME(Uploading),
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
            QList<QUrl> localImagePaths;
            QStringList imageFileHashes;
            for (const auto & pathValue : value["localImagePaths"].toArray())
                localImagePaths.append(QUrl(pathValue.toString()));
            for (const auto & hashValue : value["imageFileHashes"].toArray())
                imageFileHashes.append(hashValue.toString());
            return Tours::TourStop {
#define PROPERTY(NAME) .NAME = value[#NAME]
                PROPERTY(id).toInt(),
                PROPERTY(title).toString(),
                PROPERTY(description).toString(),
#undef PROPERTY
                .localImagePaths = localImagePaths,
                .imageFileHashes = imageFileHashes,
#define PROPERTY(NAME) .NAME = value[#NAME]
                PROPERTY(localAudioPath).toString(),
                PROPERTY(audioFileHash).toString(),
#undef PROPERTY
                .coords = QGeoCoordinate(parts.value(0).toDouble(), parts.value(1).toDouble()),
            };
        });
        if (std::ranges::none_of(m_impl->tours, [&](const Tours::Tour & item) { return item.id == tour.id; }))
        {
            const auto row = static_cast<int>(m_impl->tours.size());
            beginInsertRows({}, row, row);
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

QString TourDraftsModel::BackupDraft(qlonglong id, const QUrl & destination) const
{
    const auto it = std::ranges::find(m_impl->tours, id, &Tours::Tour::id);
    if (it == m_impl->tours.end())
        return tr("This draft no longer exists.");

    const auto path = QQmlFile::urlToLocalFileOrQrc(destination);
    if (path.isEmpty() || (!destination.isLocalFile() && destination.scheme() != "content"))
        return tr("Choose a file where the backup should be saved.");

    QFile source(Tours::GetDraftFileLocation(it->title));
    if (!source.open(QIODevice::ReadOnly))
        return tr("Could not read the draft: %1").arg(source.errorString());
    if (destination.isLocalFile() && QFileInfo(path).canonicalFilePath() == QFileInfo(source).canonicalFilePath())
        return tr("Choose a different file for the backup.");

    const auto data = source.readAll();
    if (source.error() != QFileDevice::NoError)
        return tr("Could not read the draft: %1").arg(source.errorString());

    // Android's document picker returns a content URI, which must be written
    // directly. Local filesystem destinations can be committed atomically.
    std::unique_ptr<QFileDevice> output;
    if (destination.isLocalFile())
        output = std::make_unique<QSaveFile>(path);
    else
        output = std::make_unique<QFile>(path);
    if (!output->open(QIODevice::WriteOnly) || output->write(data) != data.size())
        return tr("Could not save the backup: %1").arg(output->errorString());
    if (auto * atomicFile = qobject_cast<QSaveFile *>(output.get()))
    {
        if (!atomicFile->commit())
            return tr("Could not save the backup: %1").arg(atomicFile->errorString());
    }
    else if (!output->flush())
        return tr("Could not save the backup: %1").arg(output->errorString());
    return {};
}

QString TourDraftsModel::UpdateDraftDetails(qlonglong id, const QString & title, const QString & description, const QUrl & imageFile)
{
    const auto it = std::ranges::find(m_impl->tours, id, &Tours::Tour::id);
    if (it == m_impl->tours.end())
        return tr("This draft no longer exists.");
    if (m_impl->publishing.contains(id))
        return tr("This draft is being uploaded. Wait for it to finish.");
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
    if (m_impl->publishing.contains(id))
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
    QVariantList imageFiles;
    for (const auto & path : it->localImagePaths)
        imageFiles.append(path);
    return {
        { "id",          id                 },
        { "title",       it->title          },
        { "description", it->description    },
        { "imageFiles",  imageFiles         },
        { "audioFile",   it->localAudioPath }
    };
}

QString TourDraftsModel::UpdateDraftStop(qlonglong id, const QString & title, const QString & description, const QList<QUrl> & imageFiles, const QUrl & audioFile)
{
    auto * tour = m_impl->CurrentTour();
    if (!tour)
        return tr("This draft no longer exists.");
    if (m_impl->publishing.contains(tour->id))
        return tr("This draft is being uploaded. Wait for it to finish.");
    auto updated = *tour;
    auto it = std::ranges::find(updated.stops, id, &Tours::TourStop::id);
    if (it == updated.stops.end())
        return tr("This stop no longer exists.");
    if (title.trimmed().isEmpty())
        return tr("Enter a stop title.");
    if (it->localImagePaths != imageFiles)
    {
        QStringList hashes;
        hashes.reserve(imageFiles.size());
        for (const auto & imageFile : imageFiles)
        {
            const auto hash = Tours::ChosenFileContentHash(imageFile);
            if (!hash)
                return tr("Cannot read the selected image.");
            hashes.append(*hash);
        }
        it->localImagePaths = imageFiles;
        it->imageFileHashes = hashes;
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
    if (!tour || m_impl->publishing.contains(tour->id))
        return;
    auto & currentTour = *tour;
    currentTour.tourPath.emplace_back(coords);
}

void TourDraftsModel::AddPoint(const QGeoCoordinate & coordinate)
{
    auto * tour = m_impl->CurrentTour();
    if (!tour || m_impl->publishing.contains(tour->id))
        return;
    auto & currentTour = *tour;
    currentTour.tourPath.emplace_back(coordinate);
    currentTour.distance = StraightPathDistance(currentTour.tourPath);
    EmitCurrentTourPathEdited();
}

void TourDraftsModel::RemoveLastPoint()
{
    auto * tour = m_impl->CurrentTour();
    if (!tour || m_impl->publishing.contains(tour->id))
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

QString TourDraftsModel::CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile, const QString & tourLang)
{
    const auto imageFileHash = Tours::ChosenFileContentHash(imageFile);
    if (!imageFileHash)
    {
        LOG(ERROR) << std::format("Cannot read file: {}", imageFile.path().toStdString());
        return tr("Cannot read the selected cover image.");
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
        return tr("Could not save the draft tour.");

    StopRecording();
    AddTour(newTour);
    emit StopsUpdated();
    emit CurrentTourPathChanged();
    emit CurrentDistanceChanged();
    return {};
}

void TourDraftsModel::CreateTourStop(const QString & title, const QString & description, const QList<QUrl> & imageFiles, const QUrl & audioFile, const QGeoCoordinate & coordinate)
{
    auto * currentTour = m_impl->CurrentTour();
    if (!currentTour || m_impl->publishing.contains(currentTour->id))
        return;
    QStringList imageFileHashes;
    imageFileHashes.reserve(imageFiles.size());
    for (const auto & imageFile : imageFiles)
    {
        const auto imageFileHash = Tours::ChosenFileContentHash(imageFile);
        if (!imageFileHash)
        {
            LOG(ERROR) << std::format("Cannot read file: {}", imageFile.path().toStdString());
            return;
        }
        imageFileHashes.append(imageFileHash.value());
    }
    const auto audioFileHash = Tours::ChosenFileContentHash(audioFile);
    if (!audioFileHash)
    {
        LOG(ERROR) << std::format("Cannot read file: {}", audioFile.path().toStdString());
        return;
    }

    const auto newStopId = currentTour->stops.isEmpty() ? 0 : currentTour->stops.last().id + 1;
    const Tours::TourStop tourStop {
        .id = newStopId,
        .title = title,
        .description = description,
        .localImagePaths = imageFiles,
        .imageFileHashes = imageFileHashes,
        .localAudioPath = audioFile,
        .audioFileHash = audioFileHash.value(),
        .coords = coordinate.isValid() ? coordinate : m_impl->positionSource.Coordinate()
    };
    currentTour->stops.emplace_back(tourStop);

    auto appendedToPath { false };
    if (coordinate.isValid())
    {
        const auto isSameAsLast = !currentTour->tourPath.isEmpty() && currentTour->tourPath.last() == coordinate;
        if (!isSameAsLast)
        {
            currentTour->tourPath.emplace_back(coordinate);
            currentTour->distance = StraightPathDistance(currentTour->tourPath);
            appendedToPath = true;
        }
    }

    if (!SaveDraft(*currentTour))
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
    if (!tour || m_impl->publishing.contains(tour->id))
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
    const auto * tour = m_impl->CurrentTour();
    if (!tour || m_impl->publishing.contains(tour->id))
        return;
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

void TourDraftsModel::NotifyUploadingChanged(qlonglong id)
{
    const auto it = std::ranges::find(m_impl->tours, id, &Tours::Tour::id);
    if (it == m_impl->tours.end())
        return;
    const auto row = static_cast<int>(std::distance(m_impl->tours.begin(), it));
    emit dataChanged(index(row), index(row), { Uploading });
}

bool TourDraftsModel::StartPublishing(const Tours::Tour & tour)
{
    if (QByteArray(ADMIN_TOKEN).isEmpty())
    {
        emit ImageUploadFailed(tr("Tour uploads are unavailable because this build has no admin token configured."));
        return false;
    }
    QUrl unreadableFile;
    const auto json = Tours::TourToJson(tour, &unreadableFile);
    if (json.isEmpty())
    {
        const auto error = tr("Cannot publish the tour because this file cannot be read:\n%1\n\nChoose the image or audio file again in the draft editor, then retry.")
                               .arg(unreadableFile.toDisplayString(QUrl::PreferLocalFile));
        LOG(ERROR) << error.toStdString();
        emit ImageUploadFailed(error);
        return false;
    }

    QList<QUrl> files;
    QHash<QUrl, QString> hashes;
    const auto addFile = [&](const QUrl & file, const QString & hash) {
        if (!file.isEmpty() && !hashes.contains(file))
        {
            files.append(file);
            hashes.insert(file, hash);
        }
    };
    addFile(tour.localImagePath, json.value("imageFileHash").toString());
    const auto stops = json.value("stops").toArray();
    for (auto i = 0; i < tour.stops.size(); ++i)
    {
        const auto & stop = tour.stops.at(i);
        const auto stopJson = stops.at(i).toObject();
        const auto imageHashes = stopJson.value("imageFileHashes").toArray();
        for (auto j = 0; j < stop.localImagePaths.size(); ++j)
            addFile(stop.localImagePaths.at(j), imageHashes.at(j).toString());
        addFile(stop.localAudioPath, stopJson.value("audioFileHash").toString());
    }

    auto operation = std::make_shared<PublishOperation>();
    operation->tour = tour;
    operation->payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
    if (m_impl->editedTourId == tour.id)
        StopRecording();
    m_impl->publishing.insert(tour.id, operation);
    NotifyUploadingChanged(tour.id);
    StartAssetUploads(files, [this, operation](const QString & error) {
        if (!error.isEmpty())
            FinishPublishing(operation->tour.id, error);
        else
            SubmitTour(operation); }, hashes);
    return true;
}

void TourDraftsModel::SubmitTour(const std::shared_ptr<PublishOperation> & operation)
{
    QNetworkRequest request(QUrl("https://www.pastviewer.com/api/v1/admin/tours"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("X-Admin-Token", ADMIN_TOKEN);
    auto * reply = m_impl->networkManager.post(request, operation->payload);
    connect(reply, &QNetworkReply::finished, this, [this, operation, reply] {
        const auto body = reply->readAll();
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError || status != 201)
        {
            FinishPublishing(operation->tour.id, tr("Tour upload failed: %1").arg(RequestFailure(reply, body)));
            return;
        }
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(body, &parseError);
        const auto serverId = document.object().value("meta").toObject().value("id").toString();
        if (false
            || parseError.error != QJsonParseError::NoError
            || !document.isObject()
            || !QRegularExpression("^tour_[a-f0-9]{16}$").match(serverId).hasMatch())
        {
            FinishPublishing(operation->tour.id, tr("The server returned an invalid tour upload response."));
            return;
        }
        // Only confirmed server-draft creation permits local cleanup.
        m_impl->publishing.remove(operation->tour.id);
        const auto it = std::ranges::find(m_impl->tours, operation->tour.id, &Tours::Tour::id);
        if (it != m_impl->tours.end())
        {
            const auto row = static_cast<int>(std::distance(m_impl->tours.begin(), it));
            DeleteTourFile(*it);
            DeleteTourItem(index(row));
        }
        LOG(INFO) << "Uploaded tour draft '" << operation->tour.title.toStdString() << "'";
    });
}

void TourDraftsModel::FinishPublishing(qlonglong id, const QString & error)
{
    m_impl->publishing.remove(id);
    NotifyUploadingChanged(id);
    LOG(ERROR) << error.toStdString();
    emit ImageUploadFailed(error);
}

void TourDraftsModel::UploadAssets(const QList<QUrl> & assetFiles)
{
    if (assetFiles.isEmpty())
        return;
    if (QByteArray(ADMIN_TOKEN).isEmpty())
    {
        emit ImageUploadFailed(tr("Asset uploads are unavailable because this build has no admin token configured."));
        return;
    }
    StartAssetUploads(assetFiles, [this](const QString & error) {
        if (!error.isEmpty())
        {
            LOG(ERROR) << error.toStdString();
            emit ImageUploadFailed(error);
        }
        else
            emit AssetsUploadFinished();
    });
}

void TourDraftsModel::StartAssetUploads(const QList<QUrl> & files, UploadCompletion completion, const QHash<QUrl, QString> & expectedHashes)
{
    auto upload = std::make_shared<AssetUpload>();
    upload->files = files;
    upload->expectedHashes = expectedHashes;
    upload->completion = std::move(completion);
    UploadNextAsset(upload);
}

void TourDraftsModel::UploadNextAsset(const std::shared_ptr<AssetUpload> & upload)
{
    if (upload->next == upload->files.size())
    {
        upload->completion({});
        return;
    }
    const auto assetFile = upload->files.at(upload->next);
    const auto fileLabel = assetFile.toDisplayString(QUrl::PreferLocalFile);
    auto multipart = std::make_unique<QHttpMultiPart>(QHttpMultiPart::FormDataType);
    auto * file = new QFile(QQmlFile::urlToLocalFileOrQrc(assetFile), multipart.get());
    if (file->fileName().isEmpty() || !file->open(QIODevice::ReadOnly))
    {
        upload->completion(tr("Cannot read the file for upload:\n%1\n%2").arg(fileLabel, file->errorString()));
        return;
    }
    const auto mimeType = QMimeDatabase().mimeTypeForData(file).name();
    if (!mimeType.startsWith("image/") && !mimeType.startsWith("audio/"))
    {
        upload->completion(tr("The selected file is not an image or audio:\n%1").arg(fileLabel));
        return;
    }
    const auto hash = Tours::ChosenFileContentHash(assetFile);
    if (!hash)
    {
        upload->completion(tr("Cannot read the file for upload:\n%1").arg(fileLabel));
        return;
    }
    if (upload->expectedHashes.contains(assetFile) && upload->expectedHashes.value(assetFile) != *hash)
    {
        upload->completion(tr("The file changed while preparing the upload:\n%1\nTry publishing again.").arg(fileLabel));
        return;
    }
    const QString assetType = mimeType.startsWith("image/") ? "image" : "audio";
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
        QStringLiteral("form-data; name=\"file\"; filename=\"%1\"").arg(assetFile.fileName()));
    part.setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
    part.setBodyDevice(file);
    multipart->append(part);
    QHttpPart hashPart;
    hashPart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QStringLiteral("form-data; name=\"%1FileHash\"").arg(assetType));
    hashPart.setBody(hash->toUtf8());
    multipart->append(hashPart);

    QNetworkRequest request(QUrl("https://www.pastviewer.com/api/v1/admin/assets"));
    request.setRawHeader("X-Admin-Token", ADMIN_TOKEN);
    request.setRawHeader("X-Asset-Kind", assetType.toUtf8());
    auto * reply = m_impl->networkManager.post(request, multipart.get());
    multipart.release()->setParent(reply);
    connect(reply, &QNetworkReply::finished, this, [this, upload, reply, hash = *hash, assetType, fileLabel] {
        const auto body = reply->readAll();
        const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError || status != 201)
        {
            upload->completion(tr("File upload failed:\n%1\n%2").arg(fileLabel, RequestFailure(reply, body)));
            return;
        }
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()
            || document.object().value("id").toString() != hash
            || document.object().value("kind").toString() != assetType)
        {
            upload->completion(tr("The server returned an invalid upload response for:\n%1").arg(fileLabel));
            return;
        }
        ++upload->next;
        UploadNextAsset(upload);
    });
}
