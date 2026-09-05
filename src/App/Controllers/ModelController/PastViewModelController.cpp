#include "PastViewModelController.h"

#include <QtQml/qjsengine.h>
#include <memory>
#include <stdexcept>

#include <QGeoPositionInfo>
#include <QGuiApplication>
#include <QLocationPermission>
#include <QString>
#include <QTimer>

#include "App/Models/ClusterModel.h"
#include "glog/logging.h"

#include "App/Controllers/ModelController/PhotoNotificationCoordinator.h"
#include "App/Controllers/ModelController/PhotoNotificationSettings.h"
#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Models/BaseModel.h"
#include "App/Models/NearestObjectsModel.h"
#include "App/Models/ScreenObjectsModel.h"

namespace {
constexpr auto NEAREST_OBJECTS_ONLY = "NearestObjectsOnly";
constexpr auto HISTORY_NEAR_MODEL_TYPE = "HistoryNearModelType";
constexpr auto PROXIMITY_NOTIFICATIONS_ENABLED = "ProximityNotificationsEnabled";
constexpr auto PROXIMITY_NOTIFICATION_DISTANCE = "ProximityNotificationDistance";
constexpr auto YEARS_FROM = "YEARS_FROM";
constexpr auto YEARS_TO = "YEARS_TO";
constexpr auto YEAR_FROM_VALUE = 1800;
}

struct PastVuModelController::Impl
{
	Impl(const PastVuModelController & _pastVuModelController)
		: pastVuModelController(_pastVuModelController)
		, source(QGeoPositionInfoSource::createDefaultSource(nullptr))
		, baseModel(std::make_unique<BaseModel>(source.get()))
		, screenObjectsModel(std::make_unique<ScreenObjectsModel>(baseModel.get()))
		, nearestObjectsModel(std::make_unique<NearestObjectsModel>(screenObjectsModel.get(), source.get()))
		, clusterModelScreen(std::make_unique<ClusterModel>(screenObjectsModel.get()))
		, clusterModelNearest(std::make_unique<ClusterModel>(nearestObjectsModel.get()))
	{
		QJSEngine::setObjectOwnership(baseModel.get(), QJSEngine::CppOwnership);
		QJSEngine::setObjectOwnership(screenObjectsModel.get(), QJSEngine::CppOwnership);
		QJSEngine::setObjectOwnership(nearestObjectsModel.get(), QJSEngine::CppOwnership);
		QJSEngine::setObjectOwnership(clusterModelScreen.get(), QJSEngine::CppOwnership);
		QJSEngine::setObjectOwnership(nearestObjectsModel.get(), QJSEngine::CppOwnership);

		if (!source)
		{
			const auto message = "POSITION SOURCE EMPTY!";
			LOG(ERROR) << message;
			QTimer::singleShot(0, [&] {
				emit pastVuModelController.PositionSourceEmpty(message);
			});
			return;
		}
		positionSourceAdapter = std::make_unique<PositionSourceAdapter>(*source);
		QJSEngine::setObjectOwnership(positionSourceAdapter.get(), QJSEngine::CppOwnership);

		QLocationPermission permission;
		permission.setAccuracy(QLocationPermission::Precise);
		if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
			source->startUpdates();
	}

	const PastVuModelController & pastVuModelController;
	std::unique_ptr<QGeoPositionInfoSource> source;
	QGeoRectangle viewPort;
	std::unique_ptr<BaseModel> baseModel;
	std::unique_ptr<ScreenObjectsModel> screenObjectsModel;
	std::unique_ptr<NearestObjectsModel> nearestObjectsModel;
	std::unique_ptr<ClusterModel> clusterModelScreen;
	std::unique_ptr<ClusterModel> clusterModelNearest;
	std::unique_ptr<PositionSourceAdapter> positionSourceAdapter;
	PhotoNotificationCoordinator photoNotificationCoordinator;
	QGeoCoordinate currentCoordinate;
	QSettings settings;
	const Range defaultTimelineRange { YEAR_FROM_VALUE, QDate::currentDate().year() };
	Range userSelectedTimelineRange {
		settings.value(YEARS_FROM, defaultTimelineRange.min).toInt(),
		settings.value(YEARS_TO, defaultTimelineRange.max).toInt()
	};
};

PastVuModelController::PastVuModelController(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(*this))
{
	connect(this, &PastVuModelController::PositionPermissionGranted, m_impl->baseModel.get(), &BaseModel::OnPositionPermissionGranted);
	connect(this, &PastVuModelController::UserSelectedTimelineRangeChanged, m_impl->screenObjectsModel.get(), &ScreenObjectsModel::OnUserSelectedTimelineRangeChanged);
	connect(m_impl->baseModel.get(), &BaseModel::LoadingItems, this, &PastVuModelController::loadingItems);
	connect(m_impl->baseModel.get(), &BaseModel::ItemsLoaded, this, [&]() {
		emit itemsLoaded();
		m_impl->clusterModelScreen->OnViewportChanged(m_impl->viewPort);
		m_impl->clusterModelNearest->OnViewportChanged(m_impl->viewPort);
	});
	connect(m_impl->baseModel.get(), &BaseModel::ItemsLoaded, m_impl->clusterModelScreen.get(), [&]() {
		m_impl->clusterModelScreen->OnViewportChanged(m_impl->baseModel->GetLastKnownViewport());
	});
	connect(m_impl->baseModel.get(), &BaseModel::ItemsLoaded, m_impl->clusterModelNearest.get(), [&]() {
		m_impl->clusterModelScreen->OnViewportChanged(m_impl->baseModel->GetLastKnownViewport());
	});
	connect(m_impl->baseModel.get(), &BaseModel::ItemsLoaded, this, &PastVuModelController::EvaluatePhotoProximity);
	const auto retryPendingPhotoSelection = [this] {
		m_impl->photoNotificationCoordinator.RetryPendingPhotoSelection(GetModel(ModelType::Filtered));
	};
	connect(m_impl->baseModel.get(), &BaseModel::ItemsLoaded, this, retryPendingPhotoSelection);
	connect(m_impl->screenObjectsModel.get(), &QAbstractItemModel::modelReset, this, retryPendingPhotoSelection);
	connect(m_impl->nearestObjectsModel.get(), &QAbstractItemModel::modelReset, this, retryPendingPhotoSelection);
	connect(m_impl->source.get(), &QGeoPositionInfoSource::positionUpdated, this, [this](const QGeoPositionInfo & info) {
		m_impl->currentCoordinate = info.coordinate();
		EvaluatePhotoProximity();
	});
	connect(&m_impl->photoNotificationCoordinator, &PhotoNotificationCoordinator::PhotoAreaApproached, this, &PastVuModelController::PhotoAreaApproached);
	connect(&m_impl->photoNotificationCoordinator, &PhotoNotificationCoordinator::PhotoSelected, this, &PastVuModelController::photoSelected);
	connect(m_impl->clusterModelScreen.get(), &ClusterModel::ZoomsToDecluster, m_impl->screenObjectsModel.get(), &ScreenObjectsModel::UpdateZoomsToDecluster);
}

PastVuModelController::~PastVuModelController() = default;

void PastVuModelController::EvaluatePhotoProximity()
{
	m_impl->photoNotificationCoordinator.EvaluatePhotoProximity(
		m_impl->currentCoordinate,
		*m_impl->baseModel,
		GetProximityNotificationDistance(),
		GetProximityNotificationsEnabled());
}

QAbstractItemModel * PastVuModelController::GetModel(ModelType::Type modelType)
{
	switch (modelType)
	{
		case ModelType::Raw:
			return m_impl->baseModel.get();
		case ModelType::Filtered:
			return GetHistoryNearModelType() // @TODO think on the function name
					 ? static_cast<QAbstractItemModel *>(m_impl->screenObjectsModel.get())
					 : static_cast<QAbstractItemModel *>(m_impl->nearestObjectsModel.get());
		case ModelType::Clustered:
			return GetNearestObjectsOnly()
					 ? static_cast<QAbstractItemModel *>(m_impl->clusterModelNearest.get())
					 : static_cast<QAbstractItemModel *>(m_impl->clusterModelScreen.get());
	}
	assert(false && "Unknown model type");
}

bool PastVuModelController::SelectPhoto(int photoId)
{
	return m_impl->photoNotificationCoordinator.SelectPhoto(GetModel(ModelType::Filtered), photoId);
}

int PastVuModelController::GetNotificationDistanceMin() const
{
	return PhotoNotificationSettings::MIN_DISTANCE_METERS;
}

int PastVuModelController::GetNotificationDistanceMax() const
{
	return PhotoNotificationSettings::MAX_DISTANCE_METERS;
}

QString PastVuModelController::GetMapHostApiKey()
{
	return QString::fromUtf8(API_KEY);
}

PositionSourceAdapter * PastVuModelController::GetPositionSource()
{
	return m_impl->positionSourceAdapter.get();
}

void PastVuModelController::OnPositionPermissionGranted()
{
	emit PositionPermissionGranted();
}

bool PastVuModelController::GetNearestObjectsOnly()
{
	return m_impl->settings.value(NEAREST_OBJECTS_ONLY).toBool();
}

void PastVuModelController::SetNearestObjectsOnly(bool value)
{
	m_impl->settings.setValue(NEAREST_OBJECTS_ONLY, value);
	emit NearestObjectsOnlyChanged();
}

bool PastVuModelController::GetHistoryNearModelType()
{
	return m_impl->settings.value(HISTORY_NEAR_MODEL_TYPE).toBool();
}

void PastVuModelController::SetHistoryNearModelType(bool value)
{
	m_impl->settings.setValue(HISTORY_NEAR_MODEL_TYPE, value);
	emit HistoryNearModelChanged();
}

bool PastVuModelController::GetProximityNotificationsEnabled()
{
	return m_impl->settings.value(PROXIMITY_NOTIFICATIONS_ENABLED, true).toBool();
}

void PastVuModelController::SetProximityNotificationsEnabled(bool value)
{
	m_impl->settings.setValue(PROXIMITY_NOTIFICATIONS_ENABLED, value);
	emit ProximityNotificationsEnabledChanged();
}

int PastVuModelController::GetProximityNotificationDistance()
{
	const auto value = m_impl->settings.value(PROXIMITY_NOTIFICATION_DISTANCE, PhotoNotificationSettings::DEFAULT_DISTANCE_METERS).toInt();
	return PhotoNotificationSettings::ClampDistanceMeters(value);
}

void PastVuModelController::SetProximityNotificationDistance(int value)
{
	const auto clamped = PhotoNotificationSettings::ClampDistanceMeters(value);
	if (clamped == GetProximityNotificationDistance())
		return;

	m_impl->settings.setValue(PROXIMITY_NOTIFICATION_DISTANCE, clamped);
	emit ProximityNotificationDistanceChanged();
}

int PastVuModelController::GetZoomLevel() const
{
	return m_impl->screenObjectsModel->data({}, BaseModel::Roles::ZoomLevel).toInt();
}

void PastVuModelController::SetZoomLevel(int value)
{
	m_impl->screenObjectsModel->setData({}, value, BaseModel::Roles::ZoomLevel);
	emit ZoomLevelChanged();
}

Range PastVuModelController::GetTimelineRange() const
{
	return { m_impl->defaultTimelineRange.min, m_impl->defaultTimelineRange.max };
}

Range PastVuModelController::GetUserSelectedTimelineRange() const
{
	return m_impl->userSelectedTimelineRange;
}

void PastVuModelController::SetUserSelectedTimelineRange(const Range & range)
{
	m_impl->userSelectedTimelineRange = range;
	m_impl->settings.setValue(YEARS_FROM, range.min);
	m_impl->settings.setValue(YEARS_TO, range.max);
	emit UserSelectedTimelineRangeChanged(range);
}

void PastVuModelController::ToggleOnlyNearestObjects()
{
	SetNearestObjectsOnly(!GetNearestObjectsOnly());
	emit ModelChanged();
}

void PastVuModelController::ToggleHistoryNearYouModel()
{
	SetHistoryNearModelType(!GetHistoryNearModelType());
	emit HistoryNearModelChanged();
}

void PastVuModelController::ToggleProximityNotifications()
{
	SetProximityNotificationsEnabled(!GetProximityNotificationsEnabled());
}

void PastVuModelController::ReloadItems()
{
	m_impl->baseModel->ReloadItems();
}

void PastVuModelController::SetViewportCoordinates(const QGeoRectangle & viewport)
{
	m_impl->viewPort = viewport;
	emit m_impl->baseModel->UpdateCoords(viewport);
}
