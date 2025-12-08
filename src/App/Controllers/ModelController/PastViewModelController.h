#pragma once

#include <QAbstractItemModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QLocationPermission>
#include <QObject>
#include <QSettings>

#include <QtCore/qsize.h>
#include <QtCore/qtmetamacros.h>
#include <memory>

class PositionSourceAdapter;

class Range
{
	Q_GADGET
	Q_PROPERTY(int min MEMBER min CONSTANT)
	Q_PROPERTY(int max MEMBER max CONSTANT)

public:
	Range(int min_, int max_)
		: min(min_)
		, max(max_)
	{}

	int min;
	int max;
};

class PastVuModelController
	: public QObject
{
	Q_OBJECT

signals:
	void PositionPermissionGranted();
	void NearestObjecrtsOnlyChanged();
	void ModelChanged();
	void HistoryNearModelChanged();
	void ZoomLevelChanged();
	void YearFromChanged();
	void YearToChanged();

public:
	PastVuModelController(const QLocationPermission & permission, QSettings & settings, QObject * parent = nullptr);
	~PastVuModelController();

	Q_PROPERTY(bool nearestObjectsOnly READ GetNearestObjectsOnly WRITE SetNearestObjectsOnly NOTIFY NearestObjecrtsOnlyChanged);
	Q_PROPERTY(QAbstractItemModel * model READ GetModel NOTIFY ModelChanged);
	Q_PROPERTY(QAbstractItemModel * historyNearModel READ GetHistoryNearModel NOTIFY HistoryNearModelChanged); // @TODO: merge with model property (DRY)
	Q_PROPERTY(bool historyNearModelType READ GetHistoryNearModelType WRITE SetHistoryNearModelType NOTIFY HistoryNearModelChanged);
	Q_PROPERTY(int zoomLevel READ GetZoomLevel WRITE SetZoomLevel NOTIFY ZoomLevelChanged);
	Q_PROPERTY(int yearFrom READ GetYearFrom WRITE SetYearFrom NOTIFY YearFromChanged);
	Q_PROPERTY(int yearTo READ GetYearTo WRITE SetYearTo NOTIFY YearToChanged);
	Q_PROPERTY(Range timelineRange READ GetTimelineRange);

	Q_INVOKABLE QString GetMapHostApiKey();
	Q_INVOKABLE PositionSourceAdapter * GetPositionSource();
	Q_INVOKABLE void SetViewportCoordinates(const QGeoRectangle & viewport);
	Q_INVOKABLE void ToggleOnlyNearestObjects();
	Q_INVOKABLE void ToggleHistoryNearYouModel();

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
	int GetYearFrom() const;
	int GetYearTo() const;
	void SetYearFrom(int year);
	void SetYearTo(int year);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
