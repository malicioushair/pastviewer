#include "PastViewModelController.h"

#include <memory>

#include <QGuiApplication>
#include <QLocationPermission>
#include <stdexcept>

#include "glog/logging.h"

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Models/NearestObjectsModel.h"
#include "App/Models/ScreenObjectsModel.h"

struct PastVuModelController::Impl
{
	Impl(const QLocationPermission & permission)
		: source(QGeoPositionInfoSource::createDefaultSource(nullptr))
		, nearestObjectsModel(std::make_unique<NearestObjectsModel>(source.get()))
		, positionSourceAdapter([&] {
			if (!source)
				throw std::runtime_error("POSITION SOURCE EMPTY!");
			return std::make_unique<PositionSourceAdapter>(*source);
		}())
	{
		if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
			source->startUpdates();
	}

	std::unique_ptr<QGeoPositionInfoSource> source;
	std::unique_ptr<NearestObjectsModel> nearestObjectsModel;
	std::unique_ptr<ScreenObjectsModel> screenObjectsModel;
	std::unique_ptr<PositionSourceAdapter> positionSourceAdapter;
	bool nearestObjectsOnly { true };
};

PastVuModelController::PastVuModelController(const QLocationPermission & permission, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(permission))
{
	connect(this, &PastVuModelController::PositionPermissionGranted, m_impl->nearestObjectsModel.get(), &NearestObjectsModel::OnPositionPermissionGranted);
}

PastVuModelController::~PastVuModelController() = default;

QAbstractListModel * PastVuModelController::GetModel()
{
	return m_impl->nearestObjectsModel.get();
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
	return m_impl->nearestObjectsOnly;
}

void PastVuModelController::SetNearestObjectsOnly(bool value)
{
	m_impl->nearestObjectsOnly = value;
	emit NearestObjecrtsOnlyChanged();
}
