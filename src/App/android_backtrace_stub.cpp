// NOTE: On Android < 33, libc has no backtrace() APIs.
// glog expects them if built with HAVE_STACKTRACE=1.
// These stubs exist only to satisfy symbol resolution.
// Effect: glog still logs messages, but fatal stack traces are empty.
// If you need real stack traces, either:
//  * replace this file with a real execinfo implementation.
#if defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 33)

	#include <cstddef>
	#include <cstdlib>

extern "C"
{
	int backtrace(void ** buffer, int size)
	{
		return 0;
	}

	char ** backtrace_symbols(void * const * buffer, int size)
	{
		return nullptr;
	}

	void backtrace_symbols_fd(void * const * buffer, int size, int fd) {}

} // extern "C"

#endif // defined(__ANDROID__) && defined(__ANDROID_API__) && (__ANDROID_API__ < 33)
