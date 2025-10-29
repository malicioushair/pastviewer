#pragma once

#include <array>
#include <stdexcept>
#include <string_view>

#include <QString>

#include "glog/logging.h"

namespace DirectionUtils {

inline int BearingFromDirection(const QString & direction)
{
	if (direction.isEmpty())
	{
		LOG(WARNING) << "Direction cannot be empty";
		return 1;
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
		throw std::runtime_error("Unknown direction: " + direction.toStdString());

	return it->second;
}

} // namespace DirectionUtils
