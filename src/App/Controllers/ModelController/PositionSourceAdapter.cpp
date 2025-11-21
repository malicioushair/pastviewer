#include "PositionSourceAdapter.h"

#include <limits>

namespace {

// 5m is just an arbitrary number to prevent dirrection jitter
constexpr auto MIN_DISTANCE_METERS = 5.0;

enum class BearingSource
{
	None,
	Calculated,
};

}

struct PositionSourceAdapter::Impl
{
	Impl(const QGeoPositionInfoSource & source)
		: source(source)
	{}

	const QGeoPositionInfoSource & source;
	QGeoPositionInfo position;
	QGeoCoordinate previousCoordinate;
	double bearing { std::numeric_limits<double>::quiet_NaN() };
	BearingSource bearingSource { BearingSource::None };
};

PositionSourceAdapter::PositionSourceAdapter(const QGeoPositionInfoSource & source, QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>(source))
{
	connect(&m_impl->source, &QGeoPositionInfoSource::positionUpdated, this, &PositionSourceAdapter::OnPositionUpdated);
}

PositionSourceAdapter::~PositionSourceAdapter() = default;

QGeoPositionInfo PositionSourceAdapter::Position() const
{
	return m_impl->position;
}

QGeoCoordinate PositionSourceAdapter::Coordinate() const
{
	return Position().coordinate();
}

double PositionSourceAdapter::Bearing() const
{
	return m_impl->bearing;
}

void PositionSourceAdapter::OnPositionUpdated(const QGeoPositionInfo & info)
{
	const auto currentCoord = info.coordinate();
	if (m_impl->previousCoordinate.isValid() && currentCoord.isValid())
	{
		const auto distance = m_impl->previousCoordinate.distanceTo(currentCoord);

		if (distance >= MIN_DISTANCE_METERS)
		{
			m_impl->bearing = m_impl->previousCoordinate.azimuthTo(currentCoord);
			m_impl->bearingSource = BearingSource::Calculated;
			m_impl->previousCoordinate = currentCoord;
		}
	}
	else if (const auto isFirstValidPosition = currentCoord.isValid())
	{
		m_impl->previousCoordinate = currentCoord;
		m_impl->bearing = std::numeric_limits<double>::quiet_NaN(); // No bearing until we have movement
		m_impl->bearingSource = BearingSource::None;
	}

	m_impl->position = info;
	emit PositionChanged();
}
