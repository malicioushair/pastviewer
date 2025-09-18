#include "GuiController.h"

#include <QFileInfo>
#include <QLocationPermission>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>
#include <QtGui/qguiapplication.h>
#include <exception>
#include <memory>

#include "glog/logging.h"

#include "PastViewModelController.h"

constexpr auto PATH = "PATH";

using namespace TorrentPlayer;

struct GuiController::Impl
{
	QQmlApplicationEngine engine;
	QSettings settings;
	std::unique_ptr<PastVuModelController> pastVuModelController { std::make_unique<PastVuModelController>() };
};

GuiController::GuiController(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>())
{
	m_impl->engine.rootContext()->setContextProperty("guiController", this);
	m_impl->engine.rootContext()->setContextProperty("pastVuModelController", m_impl->pastVuModelController.get());
	m_impl->engine.addImportPath("qrc:/qt/qml");
	m_impl->engine.loadFromModule("PastViewer", "Main");

	if (m_impl->engine.rootObjects().isEmpty())
	{
		LOG(ERROR) << "Failed to load QML";
		throw std::runtime_error("Failed to load QML");
	}

	QLocationPermission permission;
	permission.setAccuracy(QLocationPermission::Precise);
	switch (qApp->checkPermission(permission))
	{
		case Qt::PermissionStatus::Undetermined:
			qApp->requestPermission(permission, this, [] {
				LOG(INFO) << "QLocationPermission granted";
			});
			break;
		case Qt::PermissionStatus::Denied:
			LOG(WARNING) << "QLocationPermission denied!";
			break;
		case Qt::PermissionStatus::Granted:
			LOG(INFO) << "QLocationPermission has already been granted";
			break;
		default:
			assert(false && "We should never get to this branch!");
			throw std::runtime_error("Unknown QLocationPermission status");
	}
}

GuiController::~GuiController() = default;
