#pragma once

#include <QAbstractItemModel>
#include <QGeoPositionInfoSource>
#include <QLocationPermission>
#include <QObject>

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

public:
	PastVuModelController(const QLocationPermission & permission, QObject * parent = nullptr);
	~PastVuModelController();

	Q_PROPERTY(bool nearestObjectsOnly READ GetNearestObjectsOnly WRITE SetNearestObjectsOnly NOTIFY NearestObjecrtsOnlyChanged);

	Q_INVOKABLE QAbstractListModel * GetModel();
	Q_INVOKABLE QString GetMapHostApiKey();
	Q_INVOKABLE PositionSourceAdapter * GetPositionSource();

	void OnPositionPermissionGranted();

	bool GetNearestObjectsOnly();
	void SetNearestObjectsOnly(bool value);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
