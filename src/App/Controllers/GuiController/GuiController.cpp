#include "GuiController.h"

#include <QCameraDevice>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QJSEngine>
#include <QLocationPermission>
#include <QMediaDevices>
#include <QPermissions>
#include <QPointer>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QtCore/qnamespace.h>

#include "glog/logging.h"

#include "App/Controllers/GuiController/TipPromptTracker.h"
#include "App/Controllers/GuiController/platform/Logic.h"
#include "App/Controllers/I18nController/I18nController.h"
#include "App/Controllers/ModelController/PastViewModelController.h"
#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Utils/HoleItem.h"
#include "App/Utils/PlatformUtils.h"

using namespace PastViewer;

namespace {

QPointer<GuiController> s_guiController;

constexpr auto MAP_ONBOARDING_KEY = "MapPageIntro";
constexpr auto PHOTO_DETAILS_ONBOARDING_KEY = "PhotoDetailsIntro";
constexpr auto LAST_SHOWN_CHANGELOG_VERSION_KEY = "Changelog/LastShownVersion";

QString AppVersion()
{
	return QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH);
}

QStringList NativeTipProductIds()
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_APPLE)
	return QString::fromUtf8(TIP_PRODUCT_IDS)
		.split(QLatin1Char(','), Qt::SkipEmptyParts);
#else
	return {};
#endif
}

QUrl TipsUrl()
{
	return QUrl(QString::fromUtf8(PASTVIEWER_TIPS_URL));
}

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
	// owned by qml
	QPointer<I18nController> i18nController {
		engine.singletonInstance<I18nController *>(
			QStringLiteral("PastViewer"),
			QStringLiteral("I18nController"))
	};
	QSettings settings;
	TipPromptTracker tipPromptTracker;
	QLocationPermission locationPermission { [] {
		QLocationPermission p;
		p.setAccuracy(QLocationPermission::Precise);
		return p;
	}() };
	QLocationPermission backgroundLocationPermission;
	QCameraPermission cameraPermission {};
	// owned by qml
	QPointer<PastVuModelController> pastVuModelController { engine.singletonInstance<PastVuModelController *>(
		QStringLiteral("PastViewer"),
		QStringLiteral("PastVuModelController")) };
	std::unique_ptr<HotReloadUrlInterceptor> interceptor { std::make_unique<HotReloadUrlInterceptor>() };
	QString lastSavedImagePath;
	QVariantList tipProducts;
	bool tipProductsLoading = false;
	bool tipPurchaseInProgress = false;

	Impl()
	{
		if constexpr (Utils::IsAndroid())
			backgroundLocationPermission = [] {
				QLocationPermission p;
				p.setAccuracy(QLocationPermission::Precise);
				p.setAvailability(QLocationPermission::Always);
				return p;
			}();
	}

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

GuiController::GuiController()
	: QObject(nullptr)
	, m_impl(std::make_unique<Impl>())
{
	if (s_guiController)
		throw std::logic_error("Only one GuiController instance is supported");
	s_guiController = this;

	qmlRegisterType<HoleItem>("PastViewer", 1, 0, "HoleItem");
	qmlRegisterUncreatableType<PositionSourceAdapter>("PastViewer", 1, 0, "PositionSourceAdapter", "Cannot create PositionSourceAdapter from QML");
	qmlRegisterUncreatableType<Range>("PastViewer", 1, 0, "range", "Range is a value type");
	qmlRegisterUncreatableMetaObject(ModelType::staticMetaObject, "PastViewer", 1, 0, "ModelType", "ModelType is an enum namespace");
	qRegisterMetaType<QGeoCoordinate>();
	qRegisterMetaType<QGeoPositionInfo>();
	m_impl->engine.addImportPath("qrc:/qt/qml");
	m_impl->engine.addUrlInterceptor(m_impl->interceptor.get());
	m_impl->LoadQml();

	if (m_impl->engine.rootObjects().isEmpty())
	{
		LOG(ERROR) << "Failed to load QML";
		throw std::runtime_error("Failed to load QML");
	}

	const auto updateBackgroundLocationTracking = [this] {
		bool enabled = false;
		if constexpr (Utils::IsAndroid())
			enabled = m_impl->pastVuModelController
				   && m_impl->pastVuModelController->property("proximityNotificationsEnabled").toBool()
				   && qApp->checkPermission(m_impl->backgroundLocationPermission) == Qt::PermissionStatus::Granted;
		PlatformDependentLogic::SetBackgroundLocationTrackingEnabled(enabled);
	};
	const auto requestBackgroundLocationPermission = [this] {
		if constexpr (Utils::IsAndroid())
		{
			if (!m_impl->pastVuModelController
				|| !m_impl->pastVuModelController->property("proximityNotificationsEnabled").toBool()
				|| qApp->checkPermission(m_impl->locationPermission) != Qt::PermissionStatus::Granted)
				return;

			RequestPermission(m_impl->backgroundLocationPermission);
		}
	};

	connect(m_impl->pastVuModelController, &PastVuModelController::PositionSourceEmpty, this, [&](const QString & message) { emit showErrorDialog(message); }, Qt::QueuedConnection);
	connect(this, &GuiController::PermissionGranted, m_impl->pastVuModelController.get(), [this, requestBackgroundLocationPermission, updateBackgroundLocationTracking](const QPermission & permission) {
		if (permission.type() == QLocationPermission::staticMetaObject.metaType())
		{
			m_impl->pastVuModelController->OnPositionPermissionGranted();
			requestBackgroundLocationPermission();
		}

		updateBackgroundLocationTracking();
	});
	connect(m_impl->pastVuModelController.get(), &PastVuModelController::ProximityNotificationsEnabledChanged, this, [requestBackgroundLocationPermission, updateBackgroundLocationTracking] {
		requestBackgroundLocationPermission();
		updateBackgroundLocationTracking();
	});
	connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [requestBackgroundLocationPermission, updateBackgroundLocationTracking](Qt::ApplicationState state) {
		if (state != Qt::ApplicationActive)
			return;

		requestBackgroundLocationPermission();
		updateBackgroundLocationTracking();
	});
	connect(m_impl->pastVuModelController.get(), &PastVuModelController::PhotoAreaApproached, this, [](const QString & photoTitle, int photoId) {
		if (QGuiApplication::applicationState() == Qt::ApplicationActive)
			return;

		PlatformDependentLogic::ShowPhotoProximityNotification(GuiController::tr("Historical photo nearby"), GuiController::tr("You are near \"%1\".").arg(photoTitle), photoId);
	});

	PlatformDependentLogic::InitializeNotifications([&](int photoId) {
		if (!m_impl->pastVuModelController)
			return;

		m_impl->pastVuModelController->SelectPhoto(photoId);
	});
	PlatformDependentLogic::InitializeTipPurchases(NativeTipProductIds());
	RequestPermission(m_impl->locationPermission);
	requestBackgroundLocationPermission();
	updateBackgroundLocationTracking();
	RequestCameraPermission();
}

GuiController::~GuiController() = default;

std::unique_ptr<GuiController> GuiController::Create()
{
	return std::unique_ptr<GuiController>(new GuiController);
}

GuiController * GuiController::create(QQmlEngine * qmlEngine, QJSEngine *)
{
	if (!s_guiController)
	{
		LOG(ERROR) << "GuiController must be constructed before it is accessed from QML";
		return nullptr;
	}

	if (qmlEngine != &s_guiController->m_impl->engine)
	{
		LOG(ERROR) << "GuiController can only be accessed from its owning QML engine";
		return nullptr;
	}

	QJSEngine::setObjectOwnership(s_guiController, QJSEngine::CppOwnership);
	return s_guiController;
}

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
	return AppVersion();
}

bool GuiController::ShouldShowChangelog() const
{
	return m_impl->settings.value(LAST_SHOWN_CHANGELOG_VERSION_KEY).toString() != AppVersion();
}

void GuiController::MarkChangelogShown()
{
	m_impl->settings.setValue(LAST_SHOWN_CHANGELOG_VERSION_KEY, AppVersion());
}

bool GuiController::HasTipsUrl() const
{
	return !TipsUrl().isEmpty();
}

bool GuiController::OpenTipsUrl()
{
	const auto tipsUrl = TipsUrl();
	if (!tipsUrl.isValid() || !QDesktopServices::openUrl(tipsUrl))
	{
		LOG(ERROR) << "Failed to open PASTVIEWER_TIPS_URL: " << tipsUrl.toString().toStdString();
		emit showErrorDialog(tr("Could not open the support page."));
		return false;
	}

	m_impl->tipPromptTracker.DisableAutomaticPrompts();
	return true;
}

bool GuiController::HasTipSupport() const
{
	if (PlatformDependentLogic::SupportsNativeTipPurchases())
		return !NativeTipProductIds().isEmpty();

	return HasTipsUrl();
}

void GuiController::RequestTipFlow()
{
	if (!PlatformDependentLogic::SupportsNativeTipPurchases())
	{
		OpenTipsUrl();
		return;
	}

	if (NativeTipProductIds().isEmpty())
		return;

	m_impl->tipPromptTracker.DisableAutomaticPrompts();
	emit tipJarRequested();
}

void GuiController::LoadTipProducts()
{
	if (false
		|| !PlatformDependentLogic::SupportsNativeTipPurchases()
		|| NativeTipProductIds().isEmpty()
		|| m_impl->tipProductsLoading
		|| !m_impl->tipProducts.isEmpty())
		return;

	m_impl->tipProductsLoading = true;
	emit tipProductsLoadingChanged();

	const QPointer<GuiController> guard(this);
	PlatformDependentLogic::LoadTipProducts(
		NativeTipProductIds(),
		[guard](QVector<PlatformDependentLogic::TipProduct> products, const QString & error) {
			if (!guard)
				return;

			guard->m_impl->tipProductsLoading = false;
			emit guard->tipProductsLoadingChanged();

			if (!error.isEmpty() || products.isEmpty())
			{
				LOG(ERROR) << "Failed to load native tip products: "
						   << (error.isEmpty() ? "No configured products were returned" : error.toStdString());
				emit guard->tipOperationFailed();
				return;
			}

			QVariantList productValues;
			productValues.reserve(products.size());
			for (const auto & product : products)
			{
				QVariantMap productValue;
				productValue.insert(QStringLiteral("id"), product.id);
				productValue.insert(QStringLiteral("title"), product.title);
				productValue.insert(QStringLiteral("displayPrice"), product.displayPrice);
				productValues.push_back(productValue);
			}

			guard->m_impl->tipProducts = std::move(productValues);
			emit guard->tipProductsChanged();
		});
}

void GuiController::PurchaseTip(const QString & productId)
{
	if (false
		|| !PlatformDependentLogic::SupportsNativeTipPurchases()
		|| m_impl->tipPurchaseInProgress
		|| !NativeTipProductIds().contains(productId))
		return;

	m_impl->tipPurchaseInProgress = true;
	emit tipPurchaseInProgressChanged();

	const QPointer<GuiController> guard(this);
	PlatformDependentLogic::PurchaseTip(
		productId,
		[guard, productId](PlatformDependentLogic::TipPurchaseResult result, const QString & error) {
			if (!guard)
				return;

			guard->m_impl->tipPurchaseInProgress = false;
			emit guard->tipPurchaseInProgressChanged();

			switch (result)
			{
				case PlatformDependentLogic::TipPurchaseResult::Succeeded:
					LOG(INFO) << "Native tip purchase succeeded: " << productId.toStdString();
					guard->m_impl->tipPromptTracker.DisableAutomaticPrompts();
					emit guard->tipPurchaseSucceeded();
					return;
				case PlatformDependentLogic::TipPurchaseResult::Cancelled:
					return;
				case PlatformDependentLogic::TipPurchaseResult::Pending:
					emit guard->tipPurchasePending();
					return;
				case PlatformDependentLogic::TipPurchaseResult::Failed:
					LOG(ERROR) << "Native tip purchase failed: " << error.toStdString();
					emit guard->tipOperationFailed();
					return;
			}
		});
}

QVariantList GuiController::TipProducts() const
{
	return m_impl->tipProducts;
}

bool GuiController::TipProductsLoading() const
{
	return m_impl->tipProductsLoading;
}

bool GuiController::TipPurchaseInProgress() const
{
	return m_impl->tipPurchaseInProgress;
}

bool GuiController::ShouldShowTipsPrompt()
{
	return true
		&& HasTipSupport()
		&& IsOnboardingStepCompleted(MAP_ONBOARDING_KEY)
		&& IsOnboardingStepCompleted(PHOTO_DETAILS_ONBOARDING_KEY)
		&& m_impl->tipPromptTracker.ShouldShowPrompt(QDateTime::currentDateTimeUtc());
}

void GuiController::NotifyCameraModeLeft()
{
	if (ShouldShowTipsPrompt())
		emit tipsPromptRequested();
}

void GuiController::MarkTipsPromptShown()
{
	m_impl->tipPromptTracker.MarkPromptShown(QDateTime::currentDateTimeUtc());
}

void GuiController::DismissTipsPrompt()
{
	m_impl->tipPromptTracker.MarkPromptDismissed(QDateTime::currentDateTimeUtc());
}

bool GuiController::IsOnboardingStepCompleted(const QString & key)
{
	m_impl->settings.beginGroup("Onboarding");
	const auto res = m_impl->settings.value(key, false).toBool();
	m_impl->settings.endGroup();
	return res;
}

void GuiController::SetOnboardingStepCompleted(const QString & key)
{
	m_impl->settings.beginGroup("Onboarding");
	m_impl->settings.setValue(key, true);
	m_impl->settings.endGroup();
}

void GuiController::ResetOnboarding()
{
	m_impl->settings.remove("Onboarding");
	emit onboardingReset();
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

QString GuiController::SaveImage(const QQuickItemGrabResult * grabResult)
{
	if (!grabResult)
	{
		LOG(WARNING) << "SaveImage called with null grabResult";
		return {};
	}

	const auto timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
	const auto filename = QString("pastviewer_%1.png").arg(timestamp);
	const auto capturesPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).absoluteFilePath(qApp->applicationName());
	if (!QDir().mkpath(capturesPath))
	{
		LOG(WARNING) << "Failed to create captures directory: " << capturesPath.toStdString();
		return {};
	}

	const auto filePath = QDir(capturesPath).absoluteFilePath(filename);
	if (!grabResult->saveToFile(filePath))
	{
		LOG(WARNING) << "Failed to save screenshot to:" << filePath.toStdString();
		return {};
	}

	m_impl->lastSavedImagePath = filePath;
	m_impl->tipPromptTracker.RecordSuccessfulRecreation();
	LOG(INFO) << "Screenshot saved to:" << filePath.toStdString();

	if (Utils::IsMobile() && !SaveScreenshotToGallery(filePath))
		LOG(WARNING) << "Failed to save screenshot to gallery: " << filePath.toStdString();

	return QUrl::fromLocalFile(filePath).toString();
}

bool GuiController::ShareImage()
{
	if (m_impl->lastSavedImagePath.isEmpty())
	{
		LOG(WARNING) << "ShareImage called with no saved image";
		return false;
	}

	return PlatformDependentLogic::ShareImage(m_impl->lastSavedImagePath);
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
