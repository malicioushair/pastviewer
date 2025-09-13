#include "GuiController.h"

#include <QFileInfo>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>

#include "glog/logging.h"

constexpr auto PATH = "PATH";

using namespace TorrentPlayer;

struct GuiController::Impl
{
	Impl()
	{
	}

	QQmlApplicationEngine engine;
	QSettings settings;
};

GuiController::GuiController(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>())
{
	QQmlApplicationEngine engine;
	m_impl->engine.rootContext()->setContextProperty("guiController", this);
	m_impl->engine.addImportPath("qrc:/qt/qml");
	m_impl->engine.loadFromModule("PastViewer", "Main");

	if (m_impl->engine.rootObjects().isEmpty())
	{
		LOG(ERROR) << "Failed to load QML";
		throw std::runtime_error("Failed to load QML");
	}
}

GuiController::~GuiController() = default;
