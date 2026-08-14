#include "PhotoProximityTracker.h"

#include <cmath>
#include <limits>

namespace {

constexpr auto EXIT_DISTANCE_FACTOR = 1.5;

}

std::optional<int> PhotoProximityTracker::Update(const QGeoCoordinate & position, std::span<const PhotoArea> photoAreas, double enterDistanceMeters)
{
	if (!position.isValid() || !(enterDistanceMeters > 0.0) || !std::isfinite(enterDistanceMeters))
		return std::nullopt;

	const auto exitDistanceMeters = enterDistanceMeters * EXIT_DISTANCE_FACTOR;
	std::optional<int> nearestEnteredPhotoArea;
	auto nearestDistance = std::numeric_limits<double>::infinity();

	for (const auto & photoArea : photoAreas)
	{
		if (!photoArea.coordinate.isValid())
			continue;

		const auto distance = position.distanceTo(photoArea.coordinate);
		if (!std::isfinite(distance))
			continue;

		if (distance <= enterDistanceMeters)
		{
			const auto entered = m_activePhotoAreas.insert(photoArea.id).second;
			if (entered && distance < nearestDistance)
			{
				nearestEnteredPhotoArea = photoArea.id;
				nearestDistance = distance;
			}
		}
		else if (distance > exitDistanceMeters)
		{
			m_activePhotoAreas.erase(photoArea.id);
		}
	}

	return nearestEnteredPhotoArea;
}
