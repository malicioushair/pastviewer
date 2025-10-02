#pragma once

#include <QAbstractListModel>
#include <QVariant>
#include <memory>

class PastVuModel
	: public QAbstractListModel
{
public:
	explicit PastVuModel(QObject * parent = nullptr);
	~PastVuModel();

	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	QHash<int, QByteArray> roleNames() const override;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};