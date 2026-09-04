#pragma once

#include <memory>

#include <QGeoCoordinate>
#include <QObject>

class QAbstractItemModel;

class PhotoNotificationCoordinator
	: public QObject
{
	Q_OBJECT

public:
	explicit PhotoNotificationCoordinator(QObject * parent = nullptr);
	~PhotoNotificationCoordinator();

	void EvaluatePhotoProximity(
		const QGeoCoordinate & position,
		const QAbstractItemModel & model,
		double enterDistanceMeters,
		bool notificationsEnabled);
	bool SelectPhoto(QAbstractItemModel * model, int photoId);
	bool RetryPendingPhotoSelection(QAbstractItemModel * model);

signals:
	void PhotoAreaApproached(const QString & title, int photoId);
	void PhotoSelected(int modelIndex, const QGeoCoordinate & coordinate, bool isClustered, int zoomToDecluster);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
