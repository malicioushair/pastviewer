#include "PhotoProximityTracker.h"

#include <cmath>
#include <limits>

namespace {

constexpr auto ENTER_DISTANCE_METERS = 10.0;
constexpr auto EXIT_DISTANCE_METERS = 15.0;

}

std::optional<int> PhotoProximityTracker::Update(const QGeoCoordinate & position, std::span<const PhotoArea> photoAreas)
{
	if (!position.isValid())
		return std::nullopt;

	std::optional<int> nearestEnteredPhotoArea;
	auto nearestDistance = std::numeric_limits<double>::infinity();

	for (const auto & photoArea : photoAreas)
	{
		if (!photoArea.coordinate.isValid())
			continue;

		const auto distance = position.distanceTo(photoArea.coordinate);
		if (!std::isfinite(distance))
			continue;

		if (distance <= ENTER_DISTANCE_METERS)
		{
			const auto entered = m_activePhotoAreas.insert(photoArea.id).second;
			if (entered && distance < nearestDistance)
			{
				nearestEnteredPhotoArea = photoArea.id;
				nearestDistance = distance;
			}
		}
		else if (distance > EXIT_DISTANCE_METERS)
		{
			m_activePhotoAreas.erase(photoArea.id);
		}
	}

	return nearestEnteredPhotoArea;
}
