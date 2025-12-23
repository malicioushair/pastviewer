#pragma once

#include <QAbstractItemModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QLocationPermission>
#include <QMetaType>
#include <QObject>
#include <QSettings>

#include <QtCore/qsize.h>
#include <QtCore/qtmetamacros.h>
#include <memory>

#include "App/Utils/Range.h"

class PositionSourceAdapter;

class PastVuModelController
	: public QObject
{
	Q_OBJECT

signals:
	void PositionPermissionGranted();
	void NearestObjectsOnlyChanged();
	void ModelChanged();
	void HistoryNearModelChanged();
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
	Q_PROPERTY(QAbstractItemModel * model READ GetModel NOTIFY ModelChanged);
	Q_PROPERTY(QAbstractItemModel * historyNearModel READ GetHistoryNearModel NOTIFY HistoryNearModelChanged); // @TODO: merge with model property (DRY)
	Q_PROPERTY(bool historyNearModelType READ GetHistoryNearModelType WRITE SetHistoryNearModelType NOTIFY HistoryNearModelChanged);
	Q_PROPERTY(int zoomLevel READ GetZoomLevel WRITE SetZoomLevel NOTIFY ZoomLevelChanged);
	Q_PROPERTY(Range timelineRange READ GetTimelineRange);
	Q_PROPERTY(Range userSelectedTimelineRange READ GetUserSelectedTimelineRange WRITE SetUserSelectedTimelineRange NOTIFY UserSelectedTimelineRangeChanged);

	Q_INVOKABLE QString GetMapHostApiKey();
	Q_INVOKABLE PositionSourceAdapter * GetPositionSource();
	Q_INVOKABLE void SetViewportCoordinates(const QGeoRectangle & viewport);
	Q_INVOKABLE void ToggleOnlyNearestObjects();
	Q_INVOKABLE void ToggleHistoryNearYouModel();
	Q_INVOKABLE void ReloadItems();

	void OnPositionPermissionGranted();

	QAbstractItemModel * GetModel();
	QAbstractItemModel * GetHistoryNearModel();

private:
	bool GetNearestObjectsOnly();
	void SetNearestObjectsOnly(bool value);

	bool GetHistoryNearModelType();
	void SetHistoryNearModelType(bool value);

	int GetZoomLevel() const;
	void SetZoomLevel(int value);

	Range GetTimelineRange() const;
	Range GetUserSelectedTimelineRange() const;
	void SetUserSelectedTimelineRange(const Range & range);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
