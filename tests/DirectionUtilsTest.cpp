#include <QString>
#include <gtest/gtest.h>

#include "App/Utils/DirectionUtils.h"

class GLogEnvironment : public ::testing::Environment
{
public:
	void SetUp() override
	{
		google::InitGoogleLogging("DirectionUtilsTest");
		FLAGS_logtostderr = false;
		FLAGS_minloglevel = 2; // Only show warnings and errors
	}
};

// Register the global environment
::testing::Environment * const glog_env = ::testing::AddGlobalTestEnvironment(new GLogEnvironment);

class DirectionUtilsTest : public ::testing::Test
{
};

TEST_F(DirectionUtilsTest, NorthDirection)
{
	EXPECT_EQ(DirectionUtils::BearingFromDirection("n"), 0);
}

TEST_F(DirectionUtilsTest, NorthEastDirection)
{
	EXPECT_EQ(DirectionUtils::BearingFromDirection("ne"), 45);
}

TEST_F(DirectionUtilsTest, EastDirection)
{
	EXPECT_EQ(DirectionUtils::BearingFromDirection("e"), 90);
}

TEST_F(DirectionUtilsTest, SouthEastDirection)
{
	EXPECT_EQ(DirectionUtils::BearingFromDirection("se"), 135);
}

TEST_F(DirectionUtilsTest, SouthDirection)
{
	EXPECT_EQ(DirectionUtils::BearingFromDirection("s"), 180);
}

TEST_F(DirectionUtilsTest, SouthWestDirection)
{
	EXPECT_EQ(DirectionUtils::BearingFromDirection("sw"), 225);
}

TEST_F(DirectionUtilsTest, WestDirection)
{
	EXPECT_EQ(DirectionUtils::BearingFromDirection("w"), 270);
}

TEST_F(DirectionUtilsTest, NorthWestDirection)
{
	EXPECT_EQ(DirectionUtils::BearingFromDirection("nw"), 315);
}

TEST_F(DirectionUtilsTest, EmptyDirection)
{
	// Empty direction should return 1 and log a warning
	EXPECT_EQ(DirectionUtils::BearingFromDirection(""), 1);
}

TEST_F(DirectionUtilsTest, InvalidDirection)
{
	// Invalid direction should throw an exception
	EXPECT_THROW(DirectionUtils::BearingFromDirection("invalid"), std::runtime_error);
}

TEST_F(DirectionUtilsTest, CaseMatters)
{
	// Test that case matters (uppercase should throw)
	EXPECT_THROW(DirectionUtils::BearingFromDirection("N"), std::runtime_error);
	EXPECT_THROW(DirectionUtils::BearingFromDirection("NE"), std::runtime_error);
}
