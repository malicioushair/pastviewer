#pragma once

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QVariant>
#include <memory>

class PastVuModel
	: public QAbstractListModel
{
	Q_OBJECT

public:
	explicit PastVuModel(QGeoPositionInfoSource * positionSource, QObject * parent = nullptr);
	~PastVuModel();

	Q_PROPERTY(int count READ rowCount() NOTIFY countChanged());

signals:
	void countChanged();

public:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	QHash<int, QByteArray> roleNames() const override;

	void OnPositionPermissionGranted();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};