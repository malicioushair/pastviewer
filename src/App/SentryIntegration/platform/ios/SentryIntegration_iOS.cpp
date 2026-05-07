#include "../../SentryIntegration.h"

#include <cstdlib>
#include <execinfo.h>
#include <mach-o/dyld.h>
#include <sentry.h>
#include <sstream>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

#include "glog/logging.h"

namespace SentryIntegration {

namespace {

constexpr auto LEVEL = "level";

std::string ResolveExecutablePath()
{
	uint32_t size = 0;
	if (_NSGetExecutablePath(nullptr, &size) != -1 || size == 0)
		return {};
	std::string path(size, '\0');
	if (_NSGetExecutablePath(path.data(), &size) != 0)
		return {};
	path.resize(std::strlen(path.c_str()));
	return path;
}

QString ResolveCaBundlePath(const QString & executableDir)
{
	if (executableDir.isEmpty())
		return {};

	const QStringList candidates = {
		QDir(executableDir).absoluteFilePath("certs/curl/cacert.pem"),
		QDir(executableDir).absoluteFilePath("Resources/certs/curl/cacert.pem"),
		QDir(executableDir).absoluteFilePath("cacert.pem"),
		QDir(executableDir).absoluteFilePath("Resources/cacert.pem"),
		QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("certs/curl/cacert.pem"),
		QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("Resources/certs/curl/cacert.pem")
	};

	for (const auto & path : candidates)
	{
		const QFileInfo info(path);
		if (info.exists() && info.isFile() && info.isReadable())
			return path;
	}

	return {};
}

class SentryIOS : public ISentry
{
private:
	int m_initResult = -1;

public:
	bool Initialize(const QString & release) override
	{
		const auto appDataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
		const auto sentryDbPath = QDir(appDataDir).absoluteFilePath(".sentry-native");

		QDir().mkpath(appDataDir);

		auto * options = sentry_options_new();
		sentry_options_set_dsn(options, SENTRY_DSN);
		sentry_options_set_database_path(options, sentryDbPath.toUtf8().constData());
		sentry_options_set_release(options, release.toUtf8().constData());
		sentry_options_set_sample_rate(options, 1.0);
		sentry_options_set_debug(options, 1);

		const auto executablePath = ResolveExecutablePath();
		const auto executableDir = executablePath.empty()
									 ? QString()
									 : QFileInfo(QString::fromStdString(executablePath)).absolutePath();
		const auto caBundlePath = ResolveCaBundlePath(executableDir);
		if (!caBundlePath.isEmpty())
			sentry_options_set_ca_certs(options, caBundlePath.toUtf8().constData());
		else
			LOG(WARNING) << "Sentry CA bundle not found in app bundle; HTTPS event upload may fail";
		const auto handlerPath = executableDir.isEmpty()
								   ? QString()
								   : QDir(executableDir).absoluteFilePath("crashpad_handler");
		const QFileInfo handlerInfo(handlerPath);
		const auto handlerExists = !handlerPath.isEmpty()
								&& handlerInfo.exists()
								&& handlerInfo.isFile()
								&& handlerInfo.isExecutable();
		if (handlerExists)
			sentry_options_set_handler_path(options, handlerPath.toUtf8().constData());
		else
			sentry_options_set_backend(options, nullptr);

		m_initResult = sentry_init(options);

		if (m_initResult != 0)
		{
			LOG(ERROR) << "Sentry initialization failed (return code: " << m_initResult << ")";
			LOG(ERROR) << "Events will not be sent to Sentry.";
			return false;
		}

		LOG(INFO) << "Sentry initialized successfully";
		LOG(INFO) << "Sentry database path: " << sentryDbPath.toStdString();
		if (!caBundlePath.isEmpty())
			LOG(INFO) << "Sentry CA bundle path: " << caBundlePath.toStdString();
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

		const auto exc = sentry_value_new_exception(
			type.toUtf8().constData(),
			message.toUtf8().constData());

		static constexpr auto max_frames = 128;
		void * stack_addrs[max_frames];
		const auto frame_count = backtrace(stack_addrs, max_frames);

		LOG(INFO) << "Captured " << frame_count << " stack frames";

		if (frame_count > 0)
		{
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

			char ** symbols = backtrace_symbols(stack_addrs + 1, frame_count - 1);
			if (symbols != nullptr)
			{
				std::ostringstream stackTraceStream;
				stackTraceStream << "C++ Stack trace (" << (frame_count - 1) << " frames):\n";
				for (int i = 0; i < frame_count - 1; ++i)
					stackTraceStream << "  #" << i << " " << (symbols[i] ? symbols[i] : "<unknown>") << "\n";

				const auto stackTraceStr = stackTraceStream.str();

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

SentryIOS g_iOSPlatform;

} // namespace

ISentry & GetPlatform()
{
	return g_iOSPlatform;
}

} // namespace SentryIntegration
