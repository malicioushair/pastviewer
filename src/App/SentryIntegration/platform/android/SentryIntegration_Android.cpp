#include "../../SentryIntegration.h"

#include <cxxabi.h>
#include <dlfcn.h>
#include <sstream>
#include <unwind.h>

#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#include <QString>

#include "glog/logging.h"

namespace {
struct BacktraceState
{
	void ** current;
	void ** end;
};

static _Unwind_Reason_Code UnwindCallback(struct _Unwind_Context * context, void * arg)
{
	auto * state = static_cast<BacktraceState *>(arg);
	const auto pc = _Unwind_GetIP(context);
	if (pc)
	{
		if (state->current == state->end)
			return _URC_END_OF_STACK;
		*state->current++ = reinterpret_cast<void *>(pc);
	}
	return _URC_NO_REASON;
}

// Capture stack trace on Android using libunwind
// Returns number of frames captured
size_t CaptureBacktrace(void ** buffer, size_t max)
{
	BacktraceState state = { buffer, buffer + max };
	_Unwind_Backtrace(UnwindCallback, &state);
	return state.current - buffer;
}

// Demangle C++ symbol names
std::string Demangle(const char * mangled)
{
	auto status = 0;
	auto * demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
	if (status == 0 && demangled)
	{
		std::string result(demangled);
		free(demangled);
		return result;
	}
	return mangled ? mangled : "unknown";
}

// Format stack trace as string for glog
std::string FormatStackTrace(void ** addresses, size_t count)
{
	std::ostringstream oss;
	for (size_t i = 0; i < count; ++i)
	{
		Dl_info info;
		if (dladdr(addresses[i], &info) && info.dli_sname)
		{
			const auto name = Demangle(info.dli_sname);
			oss << "  #" << i << " " << name;
			if (info.dli_fname)
				oss << " (" << info.dli_fname << ")";
			oss << "\n";
		}
		else
		{
			oss << "  #" << i << " <unknown> (" << std::hex << addresses[i] << std::dec << ")\n";
		}
	}
	return oss.str();
}
} // namespace

namespace SentryIntegration {

namespace {

class SentryAndroid : public ISentry
{
public:
	bool Initialize(const QString & release) override
	{
		// Get the Android Activity (which is also a Context)
		const auto activity = QJniObject::callStaticObjectMethod(
			"org/qtproject/qt/android/QtNative",
			"activity",
			"()Landroid/app/Activity;");

		if (!activity.isValid())
		{
			LOG(ERROR) << "Failed to get Android activity - Sentry initialization will fail";
			return false;
		}

		const auto dsn = QJniObject::fromString(SENTRY_DSN);
		const auto releaseStr = QJniObject::fromString(release);

		LOG(INFO) << "Initializing Sentry Android SDK with release: " << release.toStdString();

		// Call SentryHelper.init(context, dsn, release)
		QJniEnvironment env;
		QJniObject::callStaticMethod<void>(
			"org/qtproject/PastViewer/SentryHelper",
			"init",
			"(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V",
			activity.object<jobject>(),
			dsn.object<jstring>(),
			releaseStr.object<jstring>());

		if (env.checkAndClearExceptions())
		{
			LOG(ERROR) << "Exception occurred during Sentry initialization";
			return false;
		}

		LOG(INFO) << "Sentry Android SDK initialized successfully";
		return true;
	}

	void Shutdown() override
	{
		// Android SDK handles shutdown automatically
	}

	void AddBreadcrumb(const std::string & message, const std::string & level) override
	{
		const auto msg = QJniObject::fromString(QString::fromStdString(message));
		const auto levelObj = QJniObject::fromString(QString::fromStdString(level));

		QJniObject::callStaticMethod<void>(
			"org/qtproject/PastViewer/SentryHelper",
			"addBreadcrumb",
			"(Ljava/lang/String;Ljava/lang/String;)V",
			msg.object<jstring>(),
			levelObj.object<jstring>());
	}

	void CaptureException(const QString & message, const QString & type) override
	{
		LOG(INFO) << "CaptureException called - Message: " << message.toStdString() << ", Type: " << type.toStdString();

		// Capture C++ stack trace
		constexpr auto max_frames = 64;
		void * stack_addrs[max_frames];
		const auto frame_count = CaptureBacktrace(stack_addrs, max_frames);

		LOG(INFO) << "Captured " << frame_count << " stack frames";

		QString stackTraceStr;
		if (frame_count > 1) // Skip this function itself
		{
			// Format the stack trace
			const auto cppTrace = FormatStackTrace(stack_addrs + 1, frame_count - 1);
			stackTraceStr = QString::fromStdString(cppTrace);
			LOG(INFO) << "Formatted stack trace length: " << stackTraceStr.length() << " chars";
			LOG(INFO) << "Stack trace: "
					  << stackTraceStr.left(stackTraceStr.length()).toStdString();
		}
		else
		{
			LOG(WARNING) << "Not enough frames captured (frame_count=" << frame_count << "), stack trace will be empty";
		}

		// Call Java to capture the exception in Sentry
		const auto msg = QJniObject::fromString(message);
		const auto typeObj = QJniObject::fromString(type);
		const auto stackTraceObj = QJniObject::fromString(stackTraceStr);

		LOG(INFO) << "Calling Java captureException with stackTrace length: " << stackTraceStr.length();

		QJniObject::callStaticMethod<void>(
			"org/qtproject/PastViewer/SentryHelper",
			"captureException",
			"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
			msg.object<jstring>(),
			typeObj.object<jstring>(),
			stackTraceObj.object<jstring>());

		LOG(INFO) << "Java captureException call completed";
	}

	void Flush() override
	{
		QJniObject::callStaticMethod<void>(
			"org/qtproject/PastViewer/SentryHelper",
			"flush",
			"()V");
	}
};

SentryAndroid g_SentryAndroid;

} // namespace

ISentry & GetPlatform()
{
	return g_SentryAndroid;
}

} // namespace SentryIntegration
