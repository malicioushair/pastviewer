#include "GuiController.h"

#include <memory>
#include <string>

#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLocationPermission>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QUrl>
#include <QUrlQuery>

#include "glog/logging.h"

#include "PastViewModelController.h"

constexpr auto PATH = "PATH";

using namespace PastViewer;

namespace {

class HotReloadUrlInterceptor
	: public QQmlAbstractUrlInterceptor
{
public:
	explicit HotReloadUrlInterceptor(std::string token = {})
		: m_token(std::move(token))
	{
	}

	void SetToken(const std::string & token)
	{
		m_token = token;
	}

	QUrl intercept(const QUrl & url, DataType type) override
	{
		if (true
			&& type != QQmlAbstractUrlInterceptor::QmlFile
			&& type != QQmlAbstractUrlInterceptor::JavaScriptFile
			&& type != QQmlAbstractUrlInterceptor::QmldirFile)
			return url;

		if (m_token.empty())
			return url;

		const auto scheme = url.scheme();
		if (scheme != QStringLiteral("file") && scheme != QStringLiteral("qrc"))
			return url;

		QUrl result(url);
		QUrlQuery query(result);
		query.removeAllQueryItems("r");
		query.addQueryItem("r", QString::fromStdString(m_token));
		result.setQuery(query);

		return result;
	}

private:
	std::string m_token;
};

}

struct GuiController::Impl
{
	QQmlApplicationEngine engine;
	QSettings settings;
	std::unique_ptr<PastVuModelController> pastVuModelController { std::make_unique<PastVuModelController>() };
	std::unique_ptr<HotReloadUrlInterceptor> interceptor { std::make_unique<HotReloadUrlInterceptor>() };

	void LoadQml()
	{
		engine.
#ifndef NDEBUG
			load(MAIN_QML)
#else
			loadFromModule("PastViewer", "Main")
#endif
			;
	}
};

GuiController::GuiController(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>())
{
	m_impl->engine.rootContext()->setContextProperty("guiController", this);
	m_impl->engine.rootContext()->setContextProperty("pastVuModelController", m_impl->pastVuModelController.get());
	m_impl->engine.addImportPath("qrc:/qt/qml");
	m_impl->engine.addUrlInterceptor(m_impl->interceptor.get());
	m_impl->LoadQml();

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

void GuiController::BumpHotReloadToken()
{
	m_impl->interceptor->SetToken(QString::number(QDateTime::currentSecsSinceEpoch()).toStdString());
	m_impl->engine.clearComponentCache();
}

bool PastViewer::GuiController::IsDebug()
{
	return
#ifndef NDEBUG
		true
#else
		false
#endif
		;
}
