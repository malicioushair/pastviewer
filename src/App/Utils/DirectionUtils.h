#pragma once

#include <array>
#include <string_view>

#include <QString>

#include "glog/logging.h"

namespace DirectionUtils {

constexpr auto INCORRECT_DIRECTION = 361;

inline int BearingFromDirection(const QString & direction)
{
	if (direction.isEmpty())
	{
		LOG(WARNING) << "Direction cannot be empty";
		return INCORRECT_DIRECTION;
	}

	static constexpr std::array<std::pair<std::string_view, int>, 8> povDirectionToBearing {
		{
         { "n", 0 },
         { "ne", 45 },
         { "e", 90 },
         { "se", 135 },
         { "s", 180 },
         { "sw", 225 },
         { "w", 270 },
         { "nw", 315 },
		 }
	};

	const auto it = std::find_if(povDirectionToBearing.begin(), povDirectionToBearing.end(), [&](decltype(povDirectionToBearing)::const_reference item) {
		return item.first == direction;
	});
	if (it == povDirectionToBearing.cend())
	{
		LOG(WARNING) << "Incorrect direction: " << direction.toStdString();
		return INCORRECT_DIRECTION;
	}

	return it->second;
}

} // namespace DirectionUtils
