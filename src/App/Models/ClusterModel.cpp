#include "ClusterModel.h"

ClusterModel::ClusterModel(const QAbstractItemModel & sourceModel, QObject * parent)
	: QAbstractListModel()
	, m_sourceModel(sourceModel)
{
}

int ClusterModel::rowCount(const QModelIndex & parent) const
{
	return m_sourceModel.rowCount(parent);
}

QVariant ClusterModel::data(const QModelIndex & index, int role) const
{
	return m_sourceModel.data(index, role);
}

QHash<int, QByteArray> ClusterModel::roleNames() const
{
	return m_sourceModel.roleNames();
}
