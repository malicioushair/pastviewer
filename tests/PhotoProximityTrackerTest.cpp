#include <array>

#include <QGeoCoordinate>
#include <gtest/gtest.h>

#include "App/Controllers/ModelController/PhotoProximityTracker.h"

namespace {

const QGeoCoordinate PHOTO_COORDINATE { 44.817812, 20.456897 };

QGeoCoordinate CoordinateNorthOfPhoto(double distanceMeters)
{
	return PHOTO_COORDINATE.atDistanceAndAzimuth(distanceMeters, 0.0);
}

}

TEST(PhotoProximityTrackerTest, NotifiesWhenEnteringTenMeterRadius)
{
	PhotoProximityTracker tracker;
	const std::array photoAreas {
		PhotoArea { 42, PHOTO_COORDINATE }
	};

	EXPECT_FALSE(tracker.Update(CoordinateNorthOfPhoto(10.1), photoAreas));
	EXPECT_EQ(tracker.Update(CoordinateNorthOfPhoto(9.9), photoAreas), 42);
}

TEST(PhotoProximityTrackerTest, DoesNotRepeatWhileInside)
{
	PhotoProximityTracker tracker;
	const std::array photoAreas {
		PhotoArea { 42, PHOTO_COORDINATE }
	};

	EXPECT_EQ(tracker.Update(CoordinateNorthOfPhoto(8.0), photoAreas), 42);
	EXPECT_FALSE(tracker.Update(CoordinateNorthOfPhoto(5.0), photoAreas));
	EXPECT_FALSE(tracker.Update(CoordinateNorthOfPhoto(9.0), photoAreas));
}

TEST(PhotoProximityTrackerTest, UsesExitHysteresisBeforeRenotifying)
{
	PhotoProximityTracker tracker;
	const std::array photoAreas {
		PhotoArea { 42, PHOTO_COORDINATE }
	};

	EXPECT_EQ(tracker.Update(CoordinateNorthOfPhoto(8.0), photoAreas), 42);
	EXPECT_FALSE(tracker.Update(CoordinateNorthOfPhoto(12.0), photoAreas));
	EXPECT_FALSE(tracker.Update(CoordinateNorthOfPhoto(8.0), photoAreas));
	EXPECT_FALSE(tracker.Update(CoordinateNorthOfPhoto(15.1), photoAreas));
	EXPECT_EQ(tracker.Update(CoordinateNorthOfPhoto(8.0), photoAreas), 42);
}

TEST(PhotoProximityTrackerTest, EmitsOnlyTheNearestNewArea)
{
	PhotoProximityTracker tracker;
	const std::array photoAreas {
		PhotoArea { 1, CoordinateNorthOfPhoto(8.0) },
		PhotoArea { 2, CoordinateNorthOfPhoto(3.0) },
	};

	EXPECT_EQ(tracker.Update(PHOTO_COORDINATE, photoAreas), 2);
	EXPECT_FALSE(tracker.Update(PHOTO_COORDINATE, photoAreas));
}

TEST(PhotoProximityTrackerTest, IgnoresInvalidPositionAndPhotoCoordinates)
{
	PhotoProximityTracker tracker;
	const std::array invalidPhotoAreas {
		PhotoArea { 42, {} }
	};
	const std::array validPhotoAreas {
		PhotoArea { 42, PHOTO_COORDINATE }
	};

	EXPECT_FALSE(tracker.Update({}, validPhotoAreas));
	EXPECT_FALSE(tracker.Update(PHOTO_COORDINATE, invalidPhotoAreas));
}
