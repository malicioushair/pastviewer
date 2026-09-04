#include "PhotoNotificationCoordinator.h"

#include <optional>
#include <vector>

#include <QAbstractItemModel>

#include "glog/logging.h"

#include "App/Controllers/ModelController/PhotoProximityTracker.h"
#include "App/Models/BaseModel.h"
#include "App/Models/ScreenObjectsModel.h"

struct PhotoNotificationCoordinator::Impl
{
	PhotoProximityTracker photoProximityTracker;
	std::optional<int> pendingPhotoSelectionId;
};

PhotoNotificationCoordinator::PhotoNotificationCoordinator(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>())
{
}

PhotoNotificationCoordinator::~PhotoNotificationCoordinator() = default;

void PhotoNotificationCoordinator::EvaluatePhotoProximity(
	const QGeoCoordinate & position,
	const QAbstractItemModel & model,
	double enterDistanceMeters,
	bool notificationsEnabled)
{
	if (!position.isValid())
		return;

	std::vector<PhotoArea> photoAreas;
	photoAreas.reserve(model.rowCount());

	for (auto row = 0; row < model.rowCount(); ++row)
	{
		const auto index = model.index(row, 0);
		photoAreas.push_back({
			model.data(index, BaseModel::Roles::Cid).toInt(),
			model.data(index, BaseModel::Roles::Coordinate).value<QGeoCoordinate>(),
		});
	}

	const auto enteredPhotoId = m_impl->photoProximityTracker.Update(
		position,
		photoAreas,
		enterDistanceMeters);
	if (!enteredPhotoId || !notificationsEnabled)
		return;

	const auto enteredPhoto = model.match(
		model.index(0, 0),
		BaseModel::Roles::Cid,
		*enteredPhotoId,
		1,
		Qt::MatchExactly);
	if (enteredPhoto.isEmpty())
		return;

	emit PhotoAreaApproached(
		model.data(enteredPhoto.front(), BaseModel::Roles::Title).toString(),
		*enteredPhotoId);
}

bool PhotoNotificationCoordinator::SelectPhoto(QAbstractItemModel * model, int photoId)
{
	if (!model || model->rowCount() == 0)
	{
		m_impl->pendingPhotoSelectionId = photoId;
		return false;
	}

	const auto matches = model->match(
		model->index(0, 0),
		BaseModel::Roles::Cid,
		photoId,
		1,
		Qt::MatchExactly);
	if (matches.isEmpty())
	{
		m_impl->pendingPhotoSelectionId = photoId;
		return false;
	}

	const auto index = matches.front();
	if (!model->setData(index, true, BaseModel::Roles::Selected))
	{
		LOG(WARNING) << "Failed to select photo: " << photoId;
		return false;
	}

	m_impl->pendingPhotoSelectionId.reset();
	emit PhotoSelected(
		index.row(),
		model->data(index, BaseModel::Roles::Coordinate).value<QGeoCoordinate>(),
		model->data(index, ScreenObjectsModel::Roles::IsClustered).toBool(),
		model->data(index, ScreenObjectsModel::Roles::ZoomToDecluster).toInt());
	return true;
}

bool PhotoNotificationCoordinator::RetryPendingPhotoSelection(QAbstractItemModel * model)
{
	if (!m_impl->pendingPhotoSelectionId)
		return false;

	return SelectPhoto(model, *m_impl->pendingPhotoSelectionId);
}
