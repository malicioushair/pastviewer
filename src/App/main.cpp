#include <QDir>
#include <QGuiApplication>
#include <QQuickStyle>
#include <QScopeGuard>
#include <QStandardPaths>

#include "Controllers/GuiController/GuiController.h"
#include "SentryIntegration/SentryIntegration.h"

#include "glog/logging.h"

#if defined(__ANDROID__)
extern "C" void android_backtrace_log_status();
#endif

void InitLogging(const std::string & execName)
{
	const auto logDir = QDir(QString::fromStdString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() + "/logs"));
	if (!logDir.exists())
	{
		if (!logDir.mkpath(logDir.absolutePath()))
		{
			LOG(WARNING) << "Failed to create log directory: " << logDir.absolutePath().toStdString();
			return;
		}
	}

	FLAGS_log_dir = logDir.absolutePath().toStdString();
	FLAGS_alsologtostderr = true;
	google::InitGoogleLogging(execName.data());
	google::InstallFailureSignalHandler();
}

int main(int argc, char * argv[])
{
	// Initialize Sentry prior to everything
	SentryIntegration::InitSentry(QString("PastViewer@%1.%2.%3")
			.arg(VERSION_MAJOR)
			.arg(VERSION_MINOR)
			.arg(VERSION_PATCH));

	auto sentryShutdown = qScopeGuard([] {
		SentryIntegration::GetPlatform().Shutdown();
	});

	QCoreApplication::setOrganizationName("MyOrg");
	QCoreApplication::setApplicationName("PastViewer");

	InitLogging(argv[0]);

// Log backtrace implementation status (after glog is initialized)
#if defined(__ANDROID__)
	android_backtrace_log_status();
#endif

	QQuickStyle::setStyle("Basic");

	QGuiApplication app(argc, argv);

	PastViewer::GuiController guiController;

	LOG(INFO) << "Starting PastViewer application";
	return QGuiApplication::exec();
}
