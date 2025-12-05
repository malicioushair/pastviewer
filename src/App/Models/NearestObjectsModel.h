#pragma once

#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QSortFilterProxyModel>
#include <QVariant>
#include <QtCore/qabstractitemmodel.h>

#include "App/Utils/NonCopyMovable.h"

class ScreenObjectsModel;

class NearestObjectsModel
	: public QSortFilterProxyModel
{
	Q_OBJECT

public:
	explicit NearestObjectsModel(QAbstractListModel * sourceModel, QGeoPositionInfoSource * positionSource, QObject * parent = nullptr);
	NON_COPY_MOVABLE(NearestObjectsModel);

	~NearestObjectsModel();

	Q_PROPERTY(int count READ rowCount NOTIFY CountChanged)

signals:
	void CountChanged();

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
	bool lessThan(const QModelIndex & left, const QModelIndex & right) const override;

private slots:
	void OnPositionUpdated(const QGeoPositionInfo & info);
	void OnSourceModelChanged();

private:
	void UpdateAcceptedRows();
	double GetDistance(int sourceRow) const;

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
