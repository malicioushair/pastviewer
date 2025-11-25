#pragma once

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QVariant>

#include "App/Utils/NonCopyMovable.h"

class ScreenObjectsModel;

class NearestObjectsModel
	: public QAbstractListModel
{
	Q_OBJECT

public:
	enum Roles
	{
		// Getters
		Coordinate = Qt::UserRole + 1,
		Title,
		Photo,
		Thumbnail,
		Bearing,
		Year,

		// Setters
		Selected,
	};

	explicit NearestObjectsModel(ScreenObjectsModel * sourceModel, QGeoPositionInfoSource * positionSource, QObject * parent = nullptr);
	NON_COPY_MOVABLE(NearestObjectsModel);

	~NearestObjectsModel();

	Q_PROPERTY(int count READ rowCount NOTIFY CountChanged)

signals:
	void CountChanged();

public:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	QHash<int, QByteArray> roleNames() const override;

private slots:
	void OnPositionUpdated(const QGeoPositionInfo & info);
	void OnSourceModelChanged();

private:
	void UpdateFilteredItems();

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
