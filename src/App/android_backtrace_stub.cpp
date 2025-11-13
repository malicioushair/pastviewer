// NOTE: On Android < 14 (API < 34), libc has no backtrace() APIs.
// glog expects them if built with HAVE_STACKTRACE=1.
// This implementation tries to use the real backtrace functions if available
// (Android 14+), otherwise falls back to stubs.
// Effect: glog still logs messages, but fatal stack traces are empty on older Android versions.

#if defined(__ANDROID__)

	#include <android/api-level.h>
	#include <cstddef>
	#include <cstdlib>
	#include <dlfcn.h>

	#include "glog/logging.h"

namespace {

// Function pointers to real implementations (if available)
using BacktraceFunc = int (*)(void **, int);
using BacktraceSymbolsFunc = char ** (*)(void * const *, int);
using BacktraceSymbolsFdFunc = void (*)(void * const *, int, int);

BacktraceFunc g_real_backtrace = nullptr;
BacktraceSymbolsFunc g_real_backtrace_symbols = nullptr;
BacktraceSymbolsFdFunc g_real_backtrace_symbols_fd = nullptr;

// Track which implementation is being used
bool g_using_real_backtrace = false;
bool g_using_real_backtrace_symbols = false;
bool g_using_real_backtrace_symbols_fd = false;

// Initialize function pointers at startup
struct BacktraceInit
{
	BacktraceInit()
	{
		// Try to resolve real backtrace functions from libc
		void * handle = dlopen("libc.so", RTLD_LAZY);
		if (handle)
		{
			g_real_backtrace = reinterpret_cast<BacktraceFunc>(
				dlsym(handle, "backtrace"));
			g_real_backtrace_symbols = reinterpret_cast<BacktraceSymbolsFunc>(
				dlsym(handle, "backtrace_symbols"));
			g_real_backtrace_symbols_fd = reinterpret_cast<BacktraceSymbolsFdFunc>(
				dlsym(handle, "backtrace_symbols_fd"));
			dlclose(handle);
		}

		// If dlsym failed, try RTLD_DEFAULT (searches already loaded libraries)
		if (!g_real_backtrace)
			g_real_backtrace = reinterpret_cast<BacktraceFunc>(
				dlsym(RTLD_DEFAULT, "backtrace"));
		if (!g_real_backtrace_symbols)
			g_real_backtrace_symbols = reinterpret_cast<BacktraceSymbolsFunc>(
				dlsym(RTLD_DEFAULT, "backtrace_symbols"));
		if (!g_real_backtrace_symbols_fd)
			g_real_backtrace_symbols_fd = reinterpret_cast<BacktraceSymbolsFdFunc>(
				dlsym(RTLD_DEFAULT, "backtrace_symbols_fd"));

		// Update flags
		g_using_real_backtrace = (g_real_backtrace != nullptr);
		g_using_real_backtrace_symbols = (g_real_backtrace_symbols != nullptr);
		g_using_real_backtrace_symbols_fd = (g_real_backtrace_symbols_fd != nullptr);
	}
};

// Global initializer - runs before main()
static BacktraceInit g_backtrace_init;

// Stub implementations
int stub_backtrace(void ** buffer, int size)
{
	(void)buffer;
	(void)size;
	return 0;
}

char ** stub_backtrace_symbols(void * const * buffer, int size)
{
	(void)buffer;
	(void)size;
	return nullptr;
}

void stub_backtrace_symbols_fd(void * const * buffer, int size, int fd)
{
	(void)buffer;
	(void)size;
	(void)fd;
}

} // anonymous namespace

// Function to query backtrace implementation status (can be called from C++ code)
extern "C" void android_backtrace_get_status(bool * using_real_backtrace,
	bool * using_real_symbols,
	bool * using_real_symbols_fd)
{
	if (using_real_backtrace)
		*using_real_backtrace = g_using_real_backtrace;
	if (using_real_symbols)
		*using_real_symbols = g_using_real_backtrace_symbols;
	if (using_real_symbols_fd)
		*using_real_symbols_fd = g_using_real_backtrace_symbols_fd;
}

// Function to log backtrace implementation status (call this after glog is initialized)
extern "C" void android_backtrace_log_status()
{
	// Get device API level
	int device_api_level = 0;
	#if __ANDROID_API__ >= 30
	device_api_level = android_get_device_api_level();
	#else
	// Fallback: use compile-time API level if runtime function not available
	device_api_level = __ANDROID_API__;
	#endif

	const char * android_version = device_api_level >= 34 ? "14+" : device_api_level >= 33 ? "13"
															  : device_api_level >= 31     ? "12"
															  : device_api_level >= 30     ? "11"
															  : device_api_level >= 29     ? "10"
															  : device_api_level >= 28     ? "9"
																						   : "8 or older";
	const char * backtrace_status = g_using_real_backtrace ? "REAL (found in libc)" : "STUB (not available)";
	const char * symbols_status = g_using_real_backtrace_symbols ? "REAL (found in libc)" : "STUB (not available)";
	const char * symbols_fd_status = g_using_real_backtrace_symbols_fd ? "REAL (found in libc)" : "STUB (not available)";

	LOG(INFO) << "Backtrace implementation status:\n"
			  << "Device API level: "
			  << device_api_level << " (Android " << android_version << ")\n"
			  << "backtrace(): " << backtrace_status << "\n"
			  << "backtrace_symbols(): " << symbols_status << "\n"
			  << "backtrace_symbols_fd(): " << symbols_fd_status;
}

extern "C"
{
	int backtrace(void ** buffer, int size)
	{
		if (g_real_backtrace)
			return g_real_backtrace(buffer, size);
		return stub_backtrace(buffer, size);
	}

	char ** backtrace_symbols(void * const * buffer, int size)
	{
		if (g_real_backtrace_symbols)
			return g_real_backtrace_symbols(buffer, size);
		return stub_backtrace_symbols(buffer, size);
	}

	void backtrace_symbols_fd(void * const * buffer, int size, int fd)
	{
		if (g_real_backtrace_symbols_fd)
			g_real_backtrace_symbols_fd(buffer, size, fd);
		else
			stub_backtrace_symbols_fd(buffer, size, fd);
	}

} // extern "C"

#endif // defined(__ANDROID__)
