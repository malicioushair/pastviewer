#include "PastViewModelController.h"

#include <QtCore/qvariant.h>
#include <memory>

#include <QGuiApplication>
#include <QLocationPermission>
#include <QString>
#include <stdexcept>

#include "glog/logging.h"

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Models/BaseModel.h"
#include "App/Models/NearestObjectsModel.h"
#include "App/Models/ScreenObjectsModel.h"

namespace {
constexpr auto NEAREST_OBJECTS_ONLY = "NearestObjectsOnly";
constexpr auto HISTORY_NEAR_MODEL_TYPE = "HistoryNearModelType";
constexpr auto YEARS_FROM = "YEARS_FROM";
constexpr auto YEARS_TO = "YEARS_TO";
constexpr auto YEAR_FROM_VALUE = 1800;
}

struct PastVuModelController::Impl
{
	Impl(const QLocationPermission & permission, QSettings & settings)
		: source(QGeoPositionInfoSource::createDefaultSource(nullptr))
		, baseModel(std::make_unique<BaseModel>(source.get()))
		, screenObjectsModel(std::make_unique<ScreenObjectsModel>(baseModel.get(), source.get()))
		, nearestObjectsModel(std::make_unique<NearestObjectsModel>(screenObjectsModel.get(), source.get()))
		, positionSourceAdapter([&] {
			if (!source)
				throw std::runtime_error("POSITION SOURCE EMPTY!");
			return std::make_unique<PositionSourceAdapter>(*source);
		}())
		, settings(settings)
	{
		if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
			source->startUpdates();
	}

	std::unique_ptr<QGeoPositionInfoSource> source;
	QGeoRectangle viewPort;
	std::unique_ptr<BaseModel> baseModel;
	std::unique_ptr<ScreenObjectsModel> screenObjectsModel;
	std::unique_ptr<NearestObjectsModel> nearestObjectsModel;
	std::unique_ptr<PositionSourceAdapter> positionSourceAdapter;
	QSettings & settings;
	const Range defaultTimelineRange { YEAR_FROM_VALUE, QDate::currentDate().year() };
	Range userSelectedTimelineRange {
		settings.value(YEARS_FROM, defaultTimelineRange.min).toInt(),
		settings.value(YEARS_TO, defaultTimelineRange.max).toInt()
	};
};

PastVuModelController::PastVuModelController(const QLocationPermission & permission, QSettings & settings, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(permission, settings))
{
	connect(this, &PastVuModelController::PositionPermissionGranted, m_impl->baseModel.get(), &BaseModel::OnPositionPermissionGranted);
	connect(this, &PastVuModelController::UserSelectedTimelineRangeChanged, m_impl->screenObjectsModel.get(), &ScreenObjectsModel::OnUserSelectedTimelineRangeChanged);
	connect(m_impl->baseModel.get(), &BaseModel::LoadingItems, this, &PastVuModelController::loadingItems);
	connect(m_impl->baseModel.get(), &BaseModel::ItemsLoaded, this, &PastVuModelController::itemsLoaded);
}

PastVuModelController::~PastVuModelController() = default;

QAbstractItemModel * PastVuModelController::GetModel()
{
	return GetNearestObjectsOnly()
			 ? static_cast<QAbstractItemModel *>(m_impl->nearestObjectsModel.get())
			 : static_cast<QAbstractItemModel *>(m_impl->screenObjectsModel.get());
}

QAbstractItemModel * PastVuModelController::GetHistoryNearModel()
{
	return GetHistoryNearModelType()
			 ? static_cast<QAbstractItemModel *>(m_impl->screenObjectsModel.get())
			 : static_cast<QAbstractItemModel *>(m_impl->nearestObjectsModel.get());
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

void PastVuModelController::ReloadItems()
{
	m_impl->baseModel->ReloadItems();
}

void PastVuModelController::SetViewportCoordinates(const QGeoRectangle & viewport)
{
	m_impl->viewPort = viewport;
	emit m_impl->baseModel->UpdateCoords(viewport);
}
