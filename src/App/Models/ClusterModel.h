#pragma once

#include <memory>

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoRectangle>

struct IndividualNode
{
	QPersistentModelIndex src; // index into ScreenObjectsModel (or BaseModel)
};

struct ClusterNode
{
	QGeoCoordinate centroid;
	QVector<QPersistentModelIndex> members; // indices into the same source model
};

using Node = std::variant<IndividualNode, ClusterNode>;

class ClusterModel
	: public QAbstractListModel
{
public:
	enum Roles
	{
		// Cluster-specific roles
		IsCluster = Qt::UserRole + 100,
		ClusterCount,
	};

	explicit ClusterModel(const QAbstractItemModel & sourceModel, QObject * parent = nullptr);
	~ClusterModel();

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