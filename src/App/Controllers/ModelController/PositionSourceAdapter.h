#pragma once

#include <memory>

#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QObject>

class PositionSourceAdapter
	: public QObject
{
	Q_OBJECT
	Q_PROPERTY(QGeoCoordinate coordinate READ Coordinate NOTIFY PositionChanged)
	Q_PROPERTY(double bearing READ Bearing NOTIFY PositionChanged)

signals:
	void PositionChanged();

public:
	explicit PositionSourceAdapter(const QGeoPositionInfoSource & source, QObject * parent = nullptr);
	~PositionSourceAdapter();

	QGeoPositionInfo Position() const;
	QGeoCoordinate Coordinate() const;
	double Bearing() const;

private slots:
	void OnPositionUpdated(const QGeoPositionInfo & info);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
