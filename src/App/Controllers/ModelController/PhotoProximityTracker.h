#pragma once

#include <optional>
#include <span>
#include <unordered_set>

#include <QGeoCoordinate>

struct PhotoArea
{
	int id;
	QGeoCoordinate coordinate;
};

class PhotoProximityTracker
{
public:
	std::optional<int> Update(const QGeoCoordinate & position, std::span<const PhotoArea> photoAreas);

private:
	std::unordered_set<int> m_activePhotoAreas;
};
