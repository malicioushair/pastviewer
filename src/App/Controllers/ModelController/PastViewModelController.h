#pragma once

#include <QtCore/qtmetamacros.h>
#include <memory>

#include <QAbstractItemModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QLocationPermission>
#include <QMetaType>
#include <QObject>
#include <QSettings>

#include "App/Utils/Range.h"

class PositionSourceAdapter;

namespace ModelType {
Q_NAMESPACE

enum Type
{
	Clustered,
	Raw,
};
Q_ENUM_NS(Type)

}

class PastVuModelController
	: public QObject
{
	Q_OBJECT

signals:
	void PositionPermissionGranted();
	void NearestObjectsOnlyChanged();
	void ModelChanged();
	void HistoryNearModelChanged();
	void ProximityNotificationsEnabledChanged();
	void ProximityNotificationDistanceChanged();
	void PhotoAreaApproached(const QString & title, int photoId);
	void photoSelected(int modelIndex, const QGeoCoordinate & coordinate, bool isClustered, int zoomToDecluster);
	void ZoomLevelChanged();
	void YearFromChanged();
	void YearToChanged();
	void UserSelectedTimelineRangeChanged(const Range & timeline);

	// @IMPORTANT: signals exposed to QML and HAVE to be in camel case
	void loadingItems();
	void itemsLoaded();

public:
	PastVuModelController(const QLocationPermission & permission, QSettings & settings, QObject * parent = nullptr);
	~PastVuModelController();

	Q_PROPERTY(bool nearestObjectsOnly READ GetNearestObjectsOnly WRITE SetNearestObjectsOnly NOTIFY NearestObjectsOnlyChanged);
	Q_PROPERTY(bool historyNearModelType READ GetHistoryNearModelType WRITE SetHistoryNearModelType NOTIFY HistoryNearModelChanged);
	Q_PROPERTY(bool proximityNotificationsEnabled READ GetProximityNotificationsEnabled WRITE SetProximityNotificationsEnabled NOTIFY ProximityNotificationsEnabledChanged);
	Q_PROPERTY(int proximityNotificationDistance READ GetProximityNotificationDistance WRITE SetProximityNotificationDistance NOTIFY ProximityNotificationDistanceChanged);
	Q_PROPERTY(int zoomLevel READ GetZoomLevel WRITE SetZoomLevel NOTIFY ZoomLevelChanged);
	Q_PROPERTY(Range timelineRange READ GetTimelineRange);
	Q_PROPERTY(Range userSelectedTimelineRange READ GetUserSelectedTimelineRange WRITE SetUserSelectedTimelineRange NOTIFY UserSelectedTimelineRangeChanged);

	Q_INVOKABLE QString GetMapHostApiKey();
	Q_INVOKABLE PositionSourceAdapter * GetPositionSource();
	Q_INVOKABLE void SetViewportCoordinates(const QGeoRectangle & viewport);
	Q_INVOKABLE void ToggleOnlyNearestObjects();
	Q_INVOKABLE void ToggleHistoryNearYouModel();
	Q_INVOKABLE void ToggleProximityNotifications();
	Q_INVOKABLE void ReloadItems();
	Q_INVOKABLE bool SelectPhoto(int photoId);
	Q_INVOKABLE int GetNotificationDistanceMin() const;
	Q_INVOKABLE int GetNotificationDistanceMax() const;

	void OnPositionPermissionGranted();

	Q_INVOKABLE QAbstractItemModel * GetModel(ModelType::Type modelType);

private:
	void EvaluatePhotoProximity();

	bool GetNearestObjectsOnly();
	void SetNearestObjectsOnly(bool value);

	bool GetHistoryNearModelType();
	void SetHistoryNearModelType(bool value);

	bool GetProximityNotificationsEnabled();
	void SetProximityNotificationsEnabled(bool value);

	int GetProximityNotificationDistance();
	void SetProximityNotificationDistance(int value);

	int GetZoomLevel() const;
	void SetZoomLevel(int value);

	Range GetTimelineRange() const;
	Range GetUserSelectedTimelineRange() const;
	void SetUserSelectedTimelineRange(const Range & range);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
