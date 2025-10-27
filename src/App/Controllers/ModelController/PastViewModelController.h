#pragma once

#include <QAbstractItemModel>
#include <QGeoPositionInfoSource>
#include <QLocationPermission>
#include <QObject>

#include <memory>

class PositionSourceAdapter;

class PastVuModelController
	: public QObject
{
	Q_OBJECT

signals:
	void PositionPermissionGranted();

public:
	PastVuModelController(const QLocationPermission & permission, QObject * parent = nullptr);
	~PastVuModelController();

	Q_INVOKABLE QAbstractListModel * GetModel();
	Q_INVOKABLE QString GetMapHostApiKey();
	Q_INVOKABLE PositionSourceAdapter * GetPositionSource();

	void OnPositionPermissionGranted();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
