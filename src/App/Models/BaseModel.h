#pragma once

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QVariant>

#include <memory>
#include <unordered_set>
#include <vector>

#include "App/Utils/NonCopyMovable.h"

// Forward declarations
class QNetworkAccessManager;

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

class BaseModel
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

		LastBaseRole,
	};

	explicit BaseModel(QGeoPositionInfoSource * positionSource, QObject * parent = nullptr);
	NON_COPY_MOVABLE(BaseModel);

	virtual ~BaseModel();

	Q_PROPERTY(int count READ rowCount() NOTIFY CountChanged());

signals:
	void CountChanged();
	void UpdateCoords(const QGeoRectangle & viewport);

public:
	int rowCount(const QModelIndex & parent = QModelIndex()) const override;
	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	QHash<int, QByteArray> roleNames() const override;

	void OnPositionPermissionGranted();

protected:
	QNetworkAccessManager * GetNetworkManager() const;
	Items & GetMutableItems();
	const Items & GetItems() const;
	std::unordered_set<int> & GetMutableSeenCids();
	const std::unordered_set<int> & GetSeenCids() const;
	QGeoPositionInfoSource * GetPositionSource() const;
	QUrl & GetMutableUrl();
	const QUrl & GetUrl() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
