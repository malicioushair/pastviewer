#pragma once

#include <memory>

#include <QGeoCoordinate>
#include <QList>
#include <QObject>
#include <qqmlintegration.h>

class PositionSourceAdapter;
class QQmlEngine;
class QJSEngine;

class TourController
	: public QObject
{
	Q_OBJECT

public:
	TourController(const PositionSourceAdapter & positionSource, QObject * parent = nullptr);
	~TourController();

	Q_PROPERTY(QList<QGeoCoordinate> tourPath READ GetTourPath NOTIFY TourPathChanged)
	Q_PROPERTY(QString distance READ GetDistance NOTIFY DistanceChanged)

signals:
	void TourPathChanged();
	void DistanceChanged();

public:
	Q_INVOKABLE void StartRecording();
	Q_INVOKABLE void PauseRecording();
	Q_INVOKABLE void UnpauseRecording();
	Q_INVOKABLE void StopRecording();

private:
	QList<QGeoCoordinate> GetTourPath() const;
	QString GetDistance() const;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};