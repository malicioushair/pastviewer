#include "GuiController.h"

#include <QCameraDevice>
#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QLocationPermission>
#include <QMediaDevices>
#include <QPermissions>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "glog/logging.h"

#include "App/Controllers/GuiController/platform/Logic.h"
#include "App/Controllers/I18nController/I18nController.h"
#include "App/Controllers/ModelController/PastViewModelController.h"
#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Utils/PlatformUtils.h"

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
	I18nController i18nController { engine };
	QSettings settings;
	QLocationPermission locationPermission { [] {
		QLocationPermission p;
		p.setAccuracy(QLocationPermission::Precise);
		return p;
	}() };
	QCameraPermission cameraPermission {};
	std::unique_ptr<PastVuModelController> pastVuModelController;
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
	try
	{
		m_impl->pastVuModelController = { std::make_unique<PastVuModelController>(m_impl->locationPermission, m_impl->settings) };
	}
	catch (const std::runtime_error & error)
	{
		LOG(ERROR) << "Failed to init PastVuModelController: " << error.what();
		QTimer::singleShot(0, this, [this, error] {
			emit showErrorDialog(QString::fromStdString(error.what()));
		});
	}

	qmlRegisterUncreatableType<PositionSourceAdapter>("PastViewer", 1, 0, "PositionSourceAdapter", "Cannot create PositionSourceAdapter from QML");
	qmlRegisterUncreatableType<Range>("PastViewer", 1, 0, "range", "Range is a value type");
	qmlRegisterUncreatableMetaObject(ModelType::staticMetaObject, "PastViewer", 1, 0, "ModelType", "ModelType is an enum namespace");
	qRegisterMetaType<QGeoCoordinate>();
	qRegisterMetaType<QGeoPositionInfo>();
	m_impl->engine.rootContext()->setContextProperty("guiController", this);
	m_impl->engine.rootContext()->setContextProperty("pastVuModelController", m_impl->pastVuModelController.get());
	m_impl->engine.rootContext()->setContextProperty("i18nController", &m_impl->i18nController);
	m_impl->engine.addImportPath("qrc:/qt/qml");
	m_impl->engine.addUrlInterceptor(m_impl->interceptor.get());
	m_impl->LoadQml();

	if (m_impl->engine.rootObjects().isEmpty())
	{
		LOG(ERROR) << "Failed to load QML";
		throw std::runtime_error("Failed to load QML");
	}

	connect(this, &GuiController::PermissionGranted, m_impl->pastVuModelController.get(), [this](const QPermission & permission) {
		if (permission.type() == QLocationPermission::staticMetaObject.metaType())
			m_impl->pastVuModelController->OnPositionPermissionGranted();
	});

	RequestPermission(m_impl->locationPermission);
	RequestCameraPermission();
}

GuiController::~GuiController() = default;

void GuiController::BumpHotReloadToken()
{
	m_impl->interceptor->SetToken(QString::number(QDateTime::currentSecsSinceEpoch()).toStdString());
	m_impl->engine.clearComponentCache();
}

bool GuiController::IsDebug()
{
	return
#ifndef NDEBUG
		true
#else
		false
#endif
		;
}

QString GuiController::GetAppVersion()
{
	return QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH);
}

void GuiController::RequestCameraPermission()
{
	QMediaDevices devices;
	if (const auto cameras = devices.videoInputs(); cameras.isEmpty())
	{
		LOG(WARNING) << "No cameras available";
		return;
	}

	RequestPermission(m_impl->cameraPermission);
}

bool GuiController::SaveScreenshotToGallery(const QString & filePath)
{
	return PlatformDependentLogic::SaveScreenshotToGallery(filePath);
}

void GuiController::SaveImage(const QQuickItemGrabResult * grabResult)
{
	if (!grabResult)
	{
		LOG(WARNING) << "SaveImage called with null grabResult";
		return;
	}

	const auto timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
	const auto filename = QString("pastviewer_%1.png").arg(timestamp);

	const auto screenshotsPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
	const auto filePath = QString("%1/%2").arg(screenshotsPath, filename);
	if (const auto saved = grabResult->saveToFile(filePath); saved)
	{
		LOG(INFO) << "Screenshot saved to:" << filePath.toStdString();
		if (Utils::IsMobile())
			SaveScreenshotToGallery(filePath);
	}
	else
	{
		LOG(WARNING) << "Failed to save screenshot to:" << filePath.toStdString();
	}
}

void GuiController::RequestPermission(const QPermission & permission)
{
	const auto permissionStatus = qApp->checkPermission(permission);
	const auto permissionName = permission.type().name();
	switch (permissionStatus)
	{
		case Qt::PermissionStatus::Undetermined:
		{
			// permissionName and permission have to be captured by value to make sure they are valid
			qApp->requestPermission(permission, this, [this, permissionName, permission] {
				LOG(INFO) << permissionName << " requested";
				if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted)
				{
					emit PermissionGranted(permission);
					LOG(INFO) << permissionName << " granted";
				}
			});
			break;
		}
		case Qt::PermissionStatus::Denied:
			LOG(WARNING) << permissionName << " denied!";
			break;
		case Qt::PermissionStatus::Granted:
			LOG(INFO) << permissionName << " has already been granted";
			break;
		default:
			assert(false && "We should never get to this branch!");
			throw std::runtime_error("Unknown permission status");
	}
}