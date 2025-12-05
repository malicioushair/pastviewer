#pragma once

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QVariant>

#include <memory>
#include <vector>

#include "App/Utils/NonCopyMovable.h"

class QNetworkAccessManager;
class QNetworkReply;

struct Item
{
	int cid { 0 };
	QGeoCoordinate coord;
	QString file;
	QString title;
	int bearing { 0 };
	int year { 0 };
	bool selected { false };
};

using Items = std::vector<Item>;

class ScreenObjectsModel
	: public QAbstractListModel
{
	Q_OBJECT

public:
	enum Roles
	{
		// Getters
		Bearing = Qt::UserRole + 1,
		Coordinate,
		Photo,
		Thumbnail,
		Title,
		Year,
		ZoomLevel,

		// Setters
		Selected,
	};

	explicit ScreenObjectsModel(QGeoPositionInfoSource * positionSource, QObject * parent = nullptr);
	NON_COPY_MOVABLE(ScreenObjectsModel);

	~ScreenObjectsModel();

	Q_PROPERTY(int count READ rowCount NOTIFY CountChanged)

signals:
	void CountChanged();
	void UpdateCoords(const QGeoRectangle & viewport);

public:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	QHash<int, QByteArray> roleNames() const override;

	void OnPositionPermissionGranted();

private slots:
	void OnNetworkReplyFinished(QNetworkReply * reply);

private:
	void ProcessPhotos(const QJsonArray & photos);
	void AddItemsToModel(const Items & newItems);

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
