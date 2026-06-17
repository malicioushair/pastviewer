#pragma once

#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QSortFilterProxyModel>
#include <QVariant>
#include <QtCore/qabstractitemmodel.h>

#include "App/Utils/NonCopyMovable.h"

class ScreenObjectsModel;

// Keep this proxy unsorted: sorting on top of another proxy model has caused
// recursive create_mapping/sort_source_rows stack overflows in production.
class NearestObjectsModel
	: public QSortFilterProxyModel
{
	Q_OBJECT

public:
	explicit NearestObjectsModel(QAbstractItemModel * sourceModel, QGeoPositionInfoSource * positionSource, QObject * parent = nullptr);
	NON_COPY_MOVABLE(NearestObjectsModel);

	~NearestObjectsModel();

	Q_PROPERTY(int count READ rowCount NOTIFY CountChanged)

signals:
	void CountChanged();

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private slots:
	void OnPositionUpdated(const QGeoPositionInfo & info);
	void OnSourceModelChanged();
	void RebuildProxyModel();

private:
	void UpdateAcceptedRows();

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
