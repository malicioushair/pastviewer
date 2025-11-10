#include "../../SentryIntegration.h"

#include <cstdlib>
#include <execinfo.h>
#include <sentry.h>
#include <sstream>

#include <QDir>
#include <QScopeGuard>
#include <QStandardPaths>

#include "glog/logging.h"

namespace SentryIntegration {

namespace {

constexpr auto LEVEL = "level";

class SentryMacOS : public ISentry
{
private:
	int m_initResult = -1;

public:
	bool Initialize(const QString & release) override
	{
		const auto appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		const auto sentryDbPath = QDir(appDataDir).absoluteFilePath(".sentry-native");

		// Ensure the directory exists
		QDir().mkpath(appDataDir);

		auto * options = sentry_options_new();
		sentry_options_set_dsn(options, SENTRY_DSN);
		sentry_options_set_database_path(options, sentryDbPath.toUtf8().constData());
		sentry_options_set_release(options, release.toUtf8().constData());
		sentry_options_set_sample_rate(options, 1.0);
		sentry_options_set_debug(options, 1);

		m_initResult = sentry_init(options);

		if (m_initResult != 0)
		{
			LOG(ERROR) << "Sentry initialization failed (return code: " << m_initResult << ")";
			LOG(ERROR) << "Events will not be sent to Sentry.";
			return false;
		}

		LOG(INFO) << "Sentry initialized successfully";
		LOG(INFO) << "Sentry database path: " << sentryDbPath.toStdString();
		return true;
	}

	void Shutdown() override
	{
		if (m_initResult == 0)
			sentry_close();
	}

	void AddBreadcrumb(const std::string & message, const std::string & level) override
	{
		const auto breadcrumb = sentry_value_new_breadcrumb("default", message.c_str());
		const auto level_val = sentry_value_new_string(level.c_str());
		sentry_value_set_by_key(breadcrumb, LEVEL, level_val);
		const auto category = sentry_value_new_string("glog");
		sentry_value_set_by_key(breadcrumb, "category", category);
		sentry_add_breadcrumb(breadcrumb);
	}

	void CaptureException(const QString & message, const QString & type) override
	{
		LOG(INFO) << "CaptureException called - Message: " << message.toStdString() << ", Type: " << type.toStdString();

		// Use sentry_value_new_exception to create an exception with proper structure
		// This creates an exception object that Sentry can properly symbolicate
		const auto exc = sentry_value_new_exception(
			type.toUtf8().constData(),
			message.toUtf8().constData());

		// Capture the current stack trace using backtrace()
		// We need to get the actual instruction pointers first
		static constexpr auto max_frames = 128;
		void * stack_addrs[max_frames];
		const auto frame_count = backtrace(stack_addrs, max_frames);

		LOG(INFO) << "Captured " << frame_count << " stack frames";

		if (frame_count > 0)
		{
			// Pass the captured addresses to Sentry
			// Skip 1 frame to exclude this function itself
			const auto stacktrace = sentry_value_new_stacktrace(
				stack_addrs + 1,
				static_cast<size_t>(frame_count - 1));

			if (!sentry_value_is_null(stacktrace) && sentry_value_get_length(stacktrace) > 0)
			{
				sentry_value_set_by_key(exc, "stacktrace", stacktrace);
				LOG(INFO) << "Stack trace attached to exception (" << sentry_value_get_length(stacktrace) << " frames)";
			}
			else if (!sentry_value_is_null(stacktrace))
			{
				sentry_value_decref(stacktrace);
			}

			// Get symbol names for breadcrumb logging
			char ** symbols = backtrace_symbols(stack_addrs + 1, frame_count - 1);
			if (symbols != nullptr)
			{
				// Format the full stack trace into a single message
				std::ostringstream stackTraceStream;
				stackTraceStream << "C++ Stack trace (" << (frame_count - 1) << " frames):\n";
				for (int i = 0; i < frame_count - 1; ++i)
					stackTraceStream << "  #" << i << " " << (symbols[i] ? symbols[i] : "<unknown>") << "\n";

				const auto stackTraceStr = stackTraceStream.str();

				// Explicitly add stacktrace it as a breadcrumb to ensure it's captured
				const auto stackTraceBreadcrumb = sentry_value_new_breadcrumb("default", stackTraceStr.c_str());
				const auto level_val = sentry_value_new_string("info");
				sentry_value_set_by_key(stackTraceBreadcrumb, LEVEL, level_val);
				const auto category = sentry_value_new_string("stacktrace");
				sentry_value_set_by_key(stackTraceBreadcrumb, "category", category);
				sentry_add_breadcrumb(stackTraceBreadcrumb);

				free(symbols);
			}
			else
			{
				LOG(WARNING) << "Failed to get symbol names for stack trace";
			}
		}
		else
		{
			LOG(WARNING) << "No stack frames captured";
		}

		const auto event = sentry_value_new_event();
		const auto exceptions = sentry_value_new_list();
		sentry_value_append(exceptions, exc);
		sentry_value_set_by_key(event, "exception", exceptions);
		sentry_value_set_by_key(event, LEVEL, sentry_value_new_string("error"));

		sentry_capture_event(event);
	}

	void Flush() override
	{
		sentry_flush(2000);
	}
};

SentryMacOS g_macOSPlatform;

} // namespace

ISentry & GetPlatform()
{
	return g_macOSPlatform;
}

} // namespace SentryIntegration
