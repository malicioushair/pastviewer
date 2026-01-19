#pragma once

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QSortFilterProxyModel>
#include <QVariant>

#include <memory>

#include "App/Models/BaseModel.h"
#include "App/Utils/NonCopyMovable.h"
#include "App/Utils/Range.h"

class QNetworkAccessManager;
class QNetworkReply;

class ScreenObjectsModel
	: public QSortFilterProxyModel
{
	Q_OBJECT

public:
	explicit ScreenObjectsModel(QAbstractListModel * sourceModel, QObject * parent = nullptr);
	NON_COPY_MOVABLE(ScreenObjectsModel);

	~ScreenObjectsModel();

	Q_PROPERTY(int count READ rowCount NOTIFY CountChanged)

	enum Roles
	{
		IsClustered = BaseModel::LastRole,
		ZoomToDecluster,

		LastRole,
	};

signals:
	void CountChanged();

public slots:
	void OnUserSelectedTimelineRangeChanged(const Range & timeline);
	void UpdateZoomsToDecluster(const QHash<int, int> & cidsToZooms);

public:
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private slots:
	void OnPositionUpdated(const QGeoPositionInfo & info);
	void OnSourceModelChanged();

private:
	void UpdateAcceptedRows();

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};