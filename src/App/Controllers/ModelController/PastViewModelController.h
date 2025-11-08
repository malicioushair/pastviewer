#pragma once

#include <QAbstractItemModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QLocationPermission>
#include <QObject>
#include <QSettings>

#include <QtCore/qtmetamacros.h>
#include <memory>

class PositionSourceAdapter;

class PastVuModelController
	: public QObject
{
	Q_OBJECT

signals:
	void PositionPermissionGranted();
	void NearestObjecrtsOnlyChanged();
	void ModelChanged();
	void ZoomLevelChanged();

public:
	PastVuModelController(const QLocationPermission & permission, QSettings & settings, QObject * parent = nullptr);
	~PastVuModelController();

	Q_PROPERTY(bool nearestObjectsOnly READ GetNearestObjectsOnly WRITE SetNearestObjectsOnly NOTIFY NearestObjecrtsOnlyChanged);
	Q_PROPERTY(QAbstractListModel * model READ GetModel NOTIFY ModelChanged);
	Q_PROPERTY(int zoomLevel READ GetZoomLevel WRITE SetZoomLevel NOTIFY ZoomLevelChanged);

	Q_INVOKABLE QString GetMapHostApiKey();
	Q_INVOKABLE PositionSourceAdapter * GetPositionSource();
	Q_INVOKABLE void SetViewportCoordinates(const QGeoRectangle & viewport);
	Q_INVOKABLE void ToggleOnlyNearestObjects();

	void OnPositionPermissionGranted();

	QAbstractListModel * GetModel();

	bool GetNearestObjectsOnly();
	void SetNearestObjectsOnly(bool value);

	int GetZoomLevel() const;
	void SetZoomLevel(int value);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
