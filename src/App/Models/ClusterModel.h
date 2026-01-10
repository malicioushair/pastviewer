#pragma once

#include <QAbstractListModel>

class ClusterModel
	: public QAbstractListModel
{
public:
	explicit ClusterModel(const QAbstractItemModel & sourceModel, QObject * parent = nullptr);

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

private:
	const QAbstractItemModel & m_sourceModel;
};