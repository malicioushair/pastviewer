#pragma once

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QVariant>

#include "App/Utils/NonCopyMovable.h"
#include "BaseModel.h"

class ScreenObjectsModel
	: public BaseModel
{
	Q_OBJECT

public:
	enum Roles
	{
		ZoomLevel = BaseModel::Roles::LastBaseRole,
	};

	explicit ScreenObjectsModel(QGeoPositionInfoSource * positionSource, QObject * parent = nullptr);
	NON_COPY_MOVABLE(ScreenObjectsModel);

	~ScreenObjectsModel();

signals:
	void UpdateCoords(const QGeoRectangle & viewport);

public:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	QHash<int, QByteArray> roleNames() const override;

	void OnPositionPermissionGranted();

private:
	struct Impl;
	std::unique_ptr<Impl> m_screenImpl;
};
