#include <QDir>
#include <QGuiApplication>
#include <QScopeGuard>
#include <QStandardPaths>

#include "Controllers/GuiController/GuiController.h"
#include "SentryIntegration/SentryIntegration.h"

#include "glog/logging.h"

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
	// Initialize Sentry with release version
	// On Android, this must happen after QGuiApplication is created to get the Activity
	const QString releaseStr = QString("PastViewer@%1.%2.%3")
								   .arg(VERSION_MAJOR)
								   .arg(VERSION_MINOR)
								   .arg(VERSION_PATCH);

	SentryIntegration::InitSentry(releaseStr);

	auto sentryShutdown = qScopeGuard([] {
		SentryIntegration::GetPlatform().Shutdown();
	});

	QCoreApplication::setOrganizationName("MyOrg");
	QCoreApplication::setApplicationName("PastViewer");

	InitLogging(argv[0]);

	QGuiApplication app(argc, argv);

	PastViewer::GuiController guiController;

	LOG(INFO) << "Starting PastViewer application";
	return QGuiApplication::exec();
}
