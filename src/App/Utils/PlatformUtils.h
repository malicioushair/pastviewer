#pragma once

namespace Utils {

constexpr bool IsMobile()
{
#if defined(__ANDROID__) || (defined(__APPLE__) && (TARGET_OS_IPHONE || TARGET_OS_SIMULATOR))
	return true;
#else
	return false;
#endif
}
}