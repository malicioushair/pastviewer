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

}
struct TourDraftsModel::Impl
{
    const PositionSourceAdapter & positionSource;
    QNetworkAccessManager networkManager {};
    QTimer timer {};
    std::vector<Tours::Tour> tours {};
    int64_t editedTourId {}; // @todo needed?
};

TourDraftsModel::TourDraftsModel(const PositionSourceAdapter & positionSource, QObject * parent)
    : QAbstractListModel(parent)
    , m_impl(std::make_unique<Impl>(positionSource))
{
	Update();

    m_impl->timer.setInterval(5000);
    connect(&m_impl->timer, &QTimer::timeout, this, [&] {
        assert(!m_impl->tours.empty());
        auto & currentTour = m_impl->tours.back();
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
        Tours::SaveDraft(m_impl->tours.back());
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
			PROPERTY(localImagePath).toString(),
			PROPERTY(imageFileHash).toString(),
#undef PROPERTY
		};
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
        else if (const auto it = std::ranges::find_if(m_impl->tours, [&](const Tours::Tour & item) { return item.id == tour.id; }); true
                                                                                                                                    && it != m_impl->tours.cend()
                                                                                                                                    && !tour.stops.empty())
        {
            const auto matchIndices = match(this->index(0, 0), Roles::Id, static_cast<qlonglong>(tour.id));
            const auto index = matchIndices.front();
            (void)setData(index, QVariant::fromValue(tour.stops.last()), Roles::AddStop);
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
	endInsertRows();
}

void TourDraftsModel::UpdateTourPath(const QGeoCoordinate & coords)
{
    auto & currentTour = m_impl->tours.back();
    currentTour.tourPath.emplace_back(coords);
}

QList<QGeoCoordinate> TourDraftsModel::GetTourPath() const
{
    const auto & currentTour = m_impl->tours.back();
    return currentTour.tourPath;
}

void TourDraftsModel::CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile)
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
    };

    if (!Tours::SaveDraft(newTour))
        return;

    m_impl->tours.emplace_back(newTour);
}

void TourDraftsModel::CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile)
{
    auto & currentTour = m_impl->tours.back();
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
        .coords = m_impl->positionSource.Coordinate()
    };
    currentTour.stops.emplace_back(tourStop);

    if (!SaveDraft(currentTour))
        return;

    emit StopsUpdated();
}

QString TourDraftsModel::GetCurrentStops() const
{
    auto & currentTour = m_impl->tours.back();
    return QString::number(currentTour.stops.size());
}

void TourDraftsModel::StartRecording()
{
    m_impl->tours.back().distance = 0;
    emit CurrentDistanceChanged();

    auto & currentTour = m_impl->tours.back();
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
    const auto & currentTour = m_impl->tours.back();
    return TourDraftsModel::tr("%1 %2").arg(QString::number(currentTour.distance, 'f', 0), "m");
}

void TourDraftsModel::DeleteTourItem(const QModelIndex & index)
{
    if (false
        || !index.isValid()
        || index.row() >= m_impl->tours.size())
        return;

    beginRemoveRows({}, index.row(), index.row());
    m_impl->tours.erase(m_impl->tours.cbegin() + index.row());
    endRemoveRows();
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
