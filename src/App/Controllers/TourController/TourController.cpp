#include "TourController.h"

#include <QGeoCoordinate>
#include <QTimer>

#include "App/Controllers/ModelController/PositionSourceAdapter.h"

struct TourController::Impl
{
	const PositionSourceAdapter & positionSource;
	QList<QGeoCoordinate> tourPath;
	double distance;
	QTimer timer;
};

TourController::TourController(const PositionSourceAdapter & positionSource, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(positionSource))
{
	m_impl->timer.setInterval(5000);
	connect(&m_impl->timer, &QTimer::timeout, this, [&] {
		const auto lastAddedPos = m_impl->tourPath.last();
		const auto currentPos = m_impl->positionSource.Coordinate();
		m_impl->distance += lastAddedPos.distanceTo(currentPos);
		emit DistanceChanged();

		m_impl->tourPath.emplace_back(currentPos);
		emit TourPathChanged();
	});
}

TourController::~TourController() = default;

void TourController::StartRecording()
{
	m_impl->tourPath.clear();
	m_impl->tourPath.emplace_back(m_impl->positionSource.Coordinate());
	emit TourPathChanged();

	m_impl->timer.start();
}

void TourController::PauseRecording()
{
	m_impl->timer.stop();
}

void TourController::UnpauseRecording()
{
	m_impl->timer.start();
}

void TourController::StopRecording()
{
	m_impl->timer.stop();
}

QString TourController::GetDistance() const
{
	return TourController::tr("%1 %2").arg(QString::number(m_impl->distance, 'f', 0), "m");
}

QList<QGeoCoordinate> TourController::GetTourPath() const
{
	return m_impl->tourPath;
}
