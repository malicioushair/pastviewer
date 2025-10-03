#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>

#include "Controllers/GuiController/GuiController.h"

#include "glog/logging.h"

void InitLogging(const std::string & execName)
{
	const auto logDir = QDir(QString::fromStdString(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString() + "/logs"));
	if (!logDir.exists())
	{
		if (!logDir.mkpath(logDir.absolutePath()))
		{
			LOG(ERROR) << "Failed to create log directory: " << logDir.absolutePath().toStdString();
			return;
		}
	}

	FLAGS_log_dir = logDir.absolutePath().toStdString();
	FLAGS_alsologtostderr = true;
	google::InitGoogleLogging(execName.data());
}

int main(int argc, char * argv[])
{
	QCoreApplication::setOrganizationName("MyOrg");
	QCoreApplication::setApplicationName("PastViewer");
	InitLogging(argv[0]);

	LOG(INFO) << "Starting PastViewer application";

	QGuiApplication app(argc, argv);
	PastViewer::GuiController guiController;
	return QGuiApplication::exec();
}
