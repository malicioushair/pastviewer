#pragma once

#include <algorithm>

namespace PhotoNotificationSettings {

constexpr auto DEFAULT_DISTANCE_METERS = 50;
constexpr auto MIN_DISTANCE_METERS = 10;
constexpr auto MAX_DISTANCE_METERS = 200;

constexpr int ClampDistanceMeters(int value)
{
	return std::clamp(value, MIN_DISTANCE_METERS, MAX_DISTANCE_METERS);
}

}
