#pragma once

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QVariant>

#include <memory>

class ScreenObjectsModel
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
		ZoomLevel,
	};

	explicit ScreenObjectsModel(QGeoPositionInfoSource * positionSource, const QGeoRectangle & viewport, QObject * parent = nullptr);
	~ScreenObjectsModel();

	Q_PROPERTY(int count READ rowCount() NOTIFY countChanged());

signals:
	void countChanged(); // @TODO PascalCase
	void UpdateCoords(const QGeoRectangle & viewport);

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
