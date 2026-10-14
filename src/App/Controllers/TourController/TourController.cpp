#include "TourController.h"

#include <memory>

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

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Models/AvailableToursModel/AvailableToursModel.h"
#include "App/Models/TourDraftsModel/TourDraftsModel.h"
#include "App/Models/TourStopsModel/TourStopsModel.h"
#include "App/Tours/Tours.h"

struct TourController::Impl
{
    const PositionSourceAdapter & positionSource;
    QList<Tours::Tour> availableTours {};
    TourDraftsModel tourDraftsModel { positionSource };
    AvailableToursModel availableToursModel {};
    TourStopsModel tourStopsModel {};
    QVariant currentTour {};
};

TourController::TourController(const PositionSourceAdapter & positionSource, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(positionSource))
{
    connect(&m_impl->tourDraftsModel, &TourDraftsModel::CurrentDistanceChanged, this, &TourController::DistanceChanged);
    connect(&m_impl->tourDraftsModel, &TourDraftsModel::CurrentTourPathChanged, this, &TourController::TourPathChanged);
    connect(&m_impl->tourDraftsModel, &TourDraftsModel::AssetUploadFinished, this, &TourController::assetUploadFinished);
    connect(&m_impl->tourDraftsModel, &TourDraftsModel::ImageUploadFailed, this, &TourController::ImageUploadFailed);
    connect(&m_impl->tourDraftsModel, &TourDraftsModel::StopsUpdated, this, &TourController::CurrentStopsChanged);
    connect(&m_impl->tourStopsModel, &TourStopsModel::SelectionChanged, this, &TourController::SelectedStopTitleChanged);
    connect(&m_impl->tourStopsModel, &TourStopsModel::SelectionChanged, this, &TourController::selectedStopChanged);
}

TourController::~TourController() = default;

void TourController::StartRecording()
{
    m_impl->tourDraftsModel.StartRecording();
}

void TourController::PauseRecording()
{
    m_impl->tourDraftsModel.PauseRecording();
}

void TourController::ResumeRecording()
{
    m_impl->tourDraftsModel.ResumeRecording();
}

void TourController::StopRecording()
{
    m_impl->tourDraftsModel.StopRecording();
}

void TourController::CreateNewTour(const QString & title, const QString & description, const QUrl & imageFile)
{
    m_impl->tourDraftsModel.CreateNewTour(title, description, imageFile);
}

void TourController::CreateTourStop(const QString & title, const QString & description, const QUrl & imageFile, const QUrl & audioFile)
{
    m_impl->tourDraftsModel.CreateTourStop(title, description, imageFile, audioFile);
}

QAbstractListModel * TourController::GetDraftsModel() const
{
	return &m_impl->tourDraftsModel;
}

QString TourController::GetDistance() const
{
    return m_impl->tourDraftsModel.GetDistance();
}

QString TourController::GetCurrentStops() const
{
    return m_impl->tourDraftsModel.GetCurrentStops();
}

QList<QGeoCoordinate> TourController::GetTourPath() const
{
    return m_impl->tourDraftsModel.GetTourPath();
}

QAbstractListModel * TourController::GetAvailableToursModel() const
{
	m_impl->availableToursModel.Update();
	QQmlEngine::setObjectOwnership(&m_impl->availableToursModel, QJSEngine::CppOwnership);
	return &m_impl->availableToursModel;
}

QAbstractListModel * TourController::GetTourStopsModel() const
{
    QQmlEngine::setObjectOwnership(&m_impl->tourStopsModel, QJSEngine::CppOwnership);
    return &m_impl->tourStopsModel;
}

void TourController::UploadAsset(const QUrl & assetFile)
{
    m_impl->tourDraftsModel.UploadAsset(assetFile);
}

QString TourController::GetSelectedStopTitle() const
{
    return m_impl->tourStopsModel.GetSelectedStopTitle();
}

void TourController::HighlightNextStop()
{
    m_impl->tourStopsModel.SelectNextStop();
}

void TourController::HighlightPrevStop()
{
    m_impl->tourStopsModel.SelectPrevStop();
}

QVariant TourController::GetCurrentTour() const
{
    return m_impl->currentTour;
}

void TourController::SetCurrentTour(const QVariant & tour)
{
    m_impl->currentTour = tour;

    if (!tour.canConvert<DisplayTour>())
    {
        LOG(ERROR) << "Invalid tour";
        return assert(false && "Invalid tour");
    }

    const auto selectedTour = tour.value<DisplayTour>();
    m_impl->tourStopsModel.SetStops(selectedTour.stops);

    emit CurrentTourChanged();
}
