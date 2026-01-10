#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoRectangle>

#include "App/Models/ScreenObjectsModel.h"

struct IndividualNode
{
	QPersistentModelIndex indexIntoSourceModel;
};

struct ClusterNode
{
	QGeoCoordinate centroid;
	QVector<QPersistentModelIndex> indicesIntoSourceModel;
};

using Node = std::variant<IndividualNode, ClusterNode>;

class ClusterModel
	: public QAbstractListModel
{
	Q_OBJECT

public:
	enum Roles
	{
		ClusterCount = ScreenObjectsModel::Roles::LastRole + 1,
		CidsInCluster,
		IsCluster,
	};

	Q_PROPERTY(int count READ rowCount NOTIFY CountChanged)

	explicit ClusterModel(QAbstractItemModel * sourceModel, QObject * parent = nullptr);
	~ClusterModel();

signals:
	void CountChanged();
	void ZoomsToDecluster(const QHash<int, int> & cidToZoom);

public:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	std::vector<Node> BuildClusters() const;

public slots:
	void OnViewportChanged(const QGeoRectangle & viewport);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};