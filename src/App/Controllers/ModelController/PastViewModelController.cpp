#include "PastViewModelController.h"

#include <memory>

#include <QGuiApplication>
#include <QLocationPermission>
#include <stdexcept>

#include "glog/logging.h"

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Models/NearestObjectsModel.h"
#include "App/Models/ScreenObjectsModel.h"

namespace {
constexpr auto NEAREST_OBJECTS_ONLY = "NearestObjectsOnly";
constexpr auto HISTORY_NEAR_MODEL_TYPE = "HistoryNearModelType";
}

struct PastVuModelController::Impl
{
	Impl(const QLocationPermission & permission, QSettings & settings)
		: source(QGeoPositionInfoSource::createDefaultSource(nullptr))
		, nearestObjectsModel(std::make_unique<NearestObjectsModel>(source.get()))
		, screenObjectsModel(std::make_unique<ScreenObjectsModel>(source.get()))
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
	std::unique_ptr<NearestObjectsModel> nearestObjectsModel;
	std::unique_ptr<ScreenObjectsModel> screenObjectsModel;
	std::unique_ptr<PositionSourceAdapter> positionSourceAdapter;
	QSettings & settings;
};

PastVuModelController::PastVuModelController(const QLocationPermission & permission, QSettings & settings, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(permission, settings))
{
	connect(this, &PastVuModelController::PositionPermissionGranted, m_impl->nearestObjectsModel.get(), &NearestObjectsModel::OnPositionPermissionGranted);
}

PastVuModelController::~PastVuModelController() = default;

QAbstractListModel * PastVuModelController::GetModel()
{
	return GetNearestObjectsOnly()
			 ? static_cast<QAbstractListModel *>(m_impl->nearestObjectsModel.get())
			 : static_cast<QAbstractListModel *>(m_impl->screenObjectsModel.get());
}

QAbstractListModel * PastVuModelController::GetHistoryNearModel()
{
	return GetHistoryNearModelType()
			 ? static_cast<QAbstractListModel *>(m_impl->screenObjectsModel.get())
			 : static_cast<QAbstractListModel *>(m_impl->nearestObjectsModel.get());
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
	emit NearestObjecrtsOnlyChanged();
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
	return m_impl->screenObjectsModel->data({}, ScreenObjectsModel::Roles::ZoomLevel).toInt();
}

void PastVuModelController::SetZoomLevel(int value)
{
	m_impl->screenObjectsModel->setData({}, value, ScreenObjectsModel::Roles::ZoomLevel);
	emit ZoomLevelChanged();
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

void PastVuModelController::SetViewportCoordinates(const QGeoRectangle & viewport)
{
	m_impl->viewPort = viewport;
	emit m_impl->screenObjectsModel->UpdateCoords(viewport);
}
