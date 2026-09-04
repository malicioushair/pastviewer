#include <gtest/gtest.h>

#include "App/Controllers/ModelController/PhotoNotificationSettings.h"

TEST(PhotoNotificationSettingsTest, ExposesExpectedDefaultAndLimits)
{
	EXPECT_EQ(PhotoNotificationSettings::DEFAULT_DISTANCE_METERS, 50);
	EXPECT_EQ(PhotoNotificationSettings::MIN_DISTANCE_METERS, 10);
	EXPECT_EQ(PhotoNotificationSettings::MAX_DISTANCE_METERS, 200);
}

TEST(PhotoNotificationSettingsTest, ClampsDistanceToSupportedRange)
{
	EXPECT_EQ(PhotoNotificationSettings::ClampDistanceMeters(9), 10);
	EXPECT_EQ(PhotoNotificationSettings::ClampDistanceMeters(10), 10);
	EXPECT_EQ(PhotoNotificationSettings::ClampDistanceMeters(75), 75);
	EXPECT_EQ(PhotoNotificationSettings::ClampDistanceMeters(200), 200);
	EXPECT_EQ(PhotoNotificationSettings::ClampDistanceMeters(201), 200);
}
