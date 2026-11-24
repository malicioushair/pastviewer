#include <gtest/gtest.h>

#include "App/Controllers/ModelController/PositionPathFilter.h"

namespace {
QGeoPositionInfo Fix(double east, int seconds)
{
    return { QGeoCoordinate(44.8, 20.4).atDistanceAndAzimuth(east, 90),
             QDateTime::fromMSecsSinceEpoch(1000000 + seconds * 1000) };
}
}

TEST(PositionPathFilterTest, FiltersJumpAndReturnUsingLookahead)
{
    PositionPathFilter filter;
    filter.Push(Fix(0, 0), 0);
    ASSERT_TRUE(filter.Take(0));
    filter.Push(Fix(30, 1), 1000);
    EXPECT_FALSE(filter.Take(2000));
    filter.Push(Fix(2, 2), 2000);
    EXPECT_FALSE(filter.Take(2000));
    const auto accepted = filter.Take(3000);
    ASSERT_TRUE(accepted);
    EXPECT_LT(accepted->coordinate().distanceTo(Fix(2, 2).coordinate()), 0.1);
}

TEST(PositionPathFilterTest, KeepsSteadyFastTravelAndRelocation)
{
    PositionPathFilter filter;
    filter.Push(Fix(0, 0), 0);
    ASSERT_TRUE(filter.Take(0));
    filter.Push(Fix(30, 1), 1000);
    filter.Push(Fix(60, 2), 2000);
    auto accepted = filter.Take(2000);
    ASSERT_TRUE(accepted);
    EXPECT_LT(accepted->coordinate().distanceTo(Fix(30, 1).coordinate()), 0.1);
    EXPECT_TRUE(filter.Take(3000));
    filter.Push(Fix(1000, 4), 4000);
    filter.Push(Fix(1002, 5), 5000);
    EXPECT_TRUE(filter.Take(5000));
}

TEST(PositionPathFilterTest, FlushesSlowSourcesAndRejectsOldFixes)
{
    PositionPathFilter filter;
    filter.Push(Fix(0, 0), 0);
    ASSERT_TRUE(filter.Take(0));
    filter.Push(Fix(2, 1), 1000);
    EXPECT_FALSE(filter.Take(1999));
    EXPECT_TRUE(filter.Take(2000));
    filter.Push(Fix(100, 2), 2000);
    EXPECT_FALSE(filter.Take(6999));
    EXPECT_TRUE(filter.Take(7000));
    filter.Push(Fix(4, 1), 8000);
    EXPECT_FALSE(filter.HasPending());
}

TEST(PositionPathFilterTest, HandlesRapidUpdatesWithoutLosingSamples)
{
    PositionPathFilter filter;
    filter.Push(Fix(0, 0), 0);
    ASSERT_TRUE(filter.Take(0));
    filter.Push(Fix(1, 1), 1000);
    filter.Push(Fix(2, 2), 1200);
    EXPECT_TRUE(filter.Take(2000));
    EXPECT_FALSE(filter.Take(2000));
    EXPECT_TRUE(filter.Take(2200));
}
