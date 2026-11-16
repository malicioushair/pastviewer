#include "GuiController.h"

#include <QCameraDevice>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QGuiApplication>
#include <QLocationPermission>
#include <QMediaDevices>
#include <QPermissions>
#include <QPointer>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlContext>
#include <QSettings>
#include <QStandardPaths>
#include <QStringLiteral>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "App/Analytics/Analytics.h"
#include "App/Controllers/GuiController/PhotoNotificationPresenter.h"
#include "App/Controllers/GuiController/TipPromptTracker.h"
#include "App/Controllers/GuiController/platform/Logic.h"
#include "App/Controllers/I18nController/I18nController.h"
#include "App/Controllers/ModelController/PastViewModelController.h"
#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Controllers/PurchaseController/PurchaseController.h"
#include "App/Controllers/TourController/TourController.h"
#include "App/Tours/DisplayTours.h"
#include "App/Utils/HoleItem.h"
#include "App/Utils/PlatformUtils.h"

#include "glog/logging.h"

using namespace PastViewer;

namespace {

constexpr auto MAP_ONBOARDING_KEY = "MapPageIntro";
constexpr auto PHOTO_DETAILS_ONBOARDING_KEY = "PhotoDetailsIntro";
constexpr auto LAST_SHOWN_CHANGELOG_VERSION_KEY = "Changelog/LastShownVersion";

QString AppVersion()
{
    return QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_PATCH);
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
    enum class LocationRequest
    {
        None,
        Foreground,
        Background,
    };

    std::unique_ptr<QQmlApplicationEngine> engine { std::make_unique<QQmlApplicationEngine>() };

    I18nController i18nController { *engine };
    PastVuModelController pastVuModelController;
    QSettings settings {};
    TipPromptTracker purchasePromptTracker {};
    QLocationPermission locationPermission { [] {
        QLocationPermission p;
        p.setAccuracy(QLocationPermission::Precise);
        return p;
    }() };
    QLocationPermission backgroundLocationPermission {};
    QCameraPermission cameraPermission {};
    std::unique_ptr<HotReloadUrlInterceptor> interceptor { std::make_unique<HotReloadUrlInterceptor>() };
    QString lastSavedImagePath {};
    TourController tourController { *pastVuModelController.GetPositionSource(), nullptr };
    Analytics analytics { ANALYTICS_TOKEN, QUrl(ANALYTICS_HOST) };
    PurchaseController purchaseController { analytics };
    LocationRequest pendingLocationRequest { LocationRequest::None };
    bool isAdmin { false };
    bool locationDisclosureRejected { false };

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
        engine->
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
    qmlRegisterSingletonInstance("I18nController", 1, 0, "I18nController", &m_impl->i18nController);
    qmlRegisterSingletonInstance("PastVuModelController", 1, 0, "PastVuModelController", &m_impl->pastVuModelController);
    qmlRegisterSingletonInstance("GuiController", 1, 0, "GuiController", this);
    qmlRegisterSingletonInstance("TourController", 1, 0, "TourController", &m_impl->tourController);
    qmlRegisterSingletonInstance("PurchaseController", 1, 0, "PurchaseController", &m_impl->purchaseController);
    qmlRegisterSingletonInstance("Analytics", 1, 0, "Analytics", &m_impl->analytics);
    qmlRegisterType<HoleItem>("GuiController", 1, 0, "HoleItem");
    qmlRegisterUncreatableType<PositionSourceAdapter>("GuiController", 1, 0, "PositionSourceAdapter", "Cannot create PositionSourceAdapter from QML");
    qmlRegisterUncreatableType<Range>("GuiController", 1, 0, "range", "Range is a value type");
    qmlRegisterUncreatableType<DisplayTourStop>("TourController", 1, 0, "DisplayTourStop", "DisplayTourStop is not meant to be created in qml, it is only passed from C++ as a signal argument");
    qmlRegisterUncreatableMetaObject(ModelType::staticMetaObject, "GuiController", 1, 0, "ModelType", "ModelType is an enum namespace");
    qRegisterMetaType<DisplayTourStop>();
    qRegisterMetaType<QGeoCoordinate>();
    qRegisterMetaType<QGeoPositionInfo>();
    m_impl->engine->addImportPath("qrc:/qt/qml");
    m_impl->engine->addUrlInterceptor(m_impl->interceptor.get());
    m_impl->LoadQml();

    QJSEngine::setObjectOwnership(this, QJSEngine::CppOwnership);
    QJSEngine::setObjectOwnership(&m_impl->i18nController, QJSEngine::CppOwnership);
    QJSEngine::setObjectOwnership(&m_impl->pastVuModelController, QJSEngine::CppOwnership);
    QJSEngine::setObjectOwnership(&m_impl->analytics, QJSEngine::CppOwnership);
    QJSEngine::setObjectOwnership(&m_impl->tourController, QJSEngine::CppOwnership);
    QJSEngine::setObjectOwnership(&m_impl->purchaseController, QJSEngine::CppOwnership);

    if (m_impl->engine->rootObjects().isEmpty())
    {
        LOG(ERROR) << "Failed to load QML";
        throw std::runtime_error("Failed to load QML");
    }

    m_impl->analytics.Track("app_opened");

    const auto updateBackgroundLocationTracking = [this] {
        bool enabled = false;
        if constexpr (Utils::IsAndroid())
            enabled = m_impl->pastVuModelController.property("proximityNotificationsEnabled").toBool()
                   && qApp->checkPermission(m_impl->backgroundLocationPermission) == Qt::PermissionStatus::Granted;
        PlatformDependentLogic::SetBackgroundLocationTrackingEnabled(enabled);
    };
    const auto requestBackgroundLocationPermission = [this] {
        if constexpr (Utils::IsAndroid())
        {
            if (!m_impl->pastVuModelController.property("proximityNotificationsEnabled").toBool()
                || qApp->checkPermission(m_impl->locationPermission) != Qt::PermissionStatus::Granted)
                return;

            if (false
                || qApp->checkPermission(m_impl->backgroundLocationPermission) != Qt::PermissionStatus::Undetermined
                || m_impl->locationDisclosureRejected
                || m_impl->pendingLocationRequest != Impl::LocationRequest::None)
                return;
            m_impl->pendingLocationRequest = Impl::LocationRequest::Background;
            emit locationDisclosureRequested();
        }
    };

    connect(&m_impl->pastVuModelController, &PastVuModelController::PositionSourceEmpty, this, [&](const QString & message) { emit showErrorDialog(message); }, Qt::QueuedConnection);
    connect(this, &GuiController::PermissionGranted, &m_impl->pastVuModelController, [this, requestBackgroundLocationPermission, updateBackgroundLocationTracking](const QPermission & permission) {
        if (permission.type() == QLocationPermission::staticMetaObject.metaType())
        {
            m_impl->pastVuModelController.OnPositionPermissionGranted();
            requestBackgroundLocationPermission();
        }

        updateBackgroundLocationTracking();
    });
    connect(&m_impl->pastVuModelController, &PastVuModelController::ProximityNotificationsEnabledChanged, this, [requestBackgroundLocationPermission, updateBackgroundLocationTracking] {
        requestBackgroundLocationPermission();
        updateBackgroundLocationTracking();
    });
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [requestBackgroundLocationPermission, updateBackgroundLocationTracking](Qt::ApplicationState state) {
        if (state != Qt::ApplicationActive)
            return;

        requestBackgroundLocationPermission();
        updateBackgroundLocationTracking();
    });
    connect(&m_impl->pastVuModelController, &PastVuModelController::PhotoAreaApproached, this, [](const QString & photoTitle, int photoId) {
        PhotoNotificationPresenter::PresentPhotoApproach(
            QGuiApplication::applicationState(),
            GuiController::tr("Historical photo nearby"),
            GuiController::tr("You are near \"%1\".").arg(photoTitle),
            photoId,
            [](QString title, QString body, int id) {
                PlatformDependentLogic::ShowPhotoProximityNotification(std::move(title), std::move(body), id);
            });
    });
    connect(&m_impl->tourController, &TourController::ShowErrorMessage, this, &GuiController::showErrorDialog);
    m_impl->purchaseController.SetTipPromptTracker(&m_impl->purchasePromptTracker);
    PlatformDependentLogic::InitializeNotifications([&](int photoId) {
        m_impl->pastVuModelController.SelectPhoto(photoId);
    });
    if constexpr (Utils::IsAndroid())
        if (qApp->checkPermission(m_impl->locationPermission) == Qt::PermissionStatus::Undetermined)
        {
            m_impl->pendingLocationRequest = Impl::LocationRequest::Foreground;
            QTimer::singleShot(0, this, [this] { emit locationDisclosureRequested(); });
        }
        else
            QTimer::singleShot(0, this, requestBackgroundLocationPermission);
    else
        RequestPermission(m_impl->locationPermission);
    updateBackgroundLocationTracking();
    RequestCameraPermission();
}

GuiController::~GuiController() = default;

void GuiController::RespondToLocationDisclosure(bool accepted)
{
    const auto request = m_impl->pendingLocationRequest;
    m_impl->pendingLocationRequest = Impl::LocationRequest::None;
    if (!accepted)
    {
        m_impl->locationDisclosureRejected = true;
        return;
    }
    if (request == Impl::LocationRequest::Foreground)
        RequestPermission(m_impl->locationPermission);
    else if (request == Impl::LocationRequest::Background)
        RequestPermission(m_impl->backgroundLocationPermission);
}

void GuiController::BumpHotReloadToken()
{
    m_impl->interceptor->SetToken(QString::number(QDateTime::currentSecsSinceEpoch()).toStdString());
    m_impl->engine->clearComponentCache();
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

bool GuiController::ShouldShowTipsPrompt()
{
    return true
        && m_impl->purchaseController.HasTipSupport()
        && IsOnboardingStepCompleted(MAP_ONBOARDING_KEY)
        && IsOnboardingStepCompleted(PHOTO_DETAILS_ONBOARDING_KEY)
        && m_impl->purchasePromptTracker.ShouldShowPrompt(QDateTime::currentDateTimeUtc());
}

void GuiController::NotifyCameraModeLeft()
{
    if (ShouldShowTipsPrompt())
        emit tipsPromptRequested();
}

void GuiController::MarkTipsPromptShown()
{
    m_impl->purchasePromptTracker.MarkPromptShown(QDateTime::currentDateTimeUtc());
}

void GuiController::DismissTipsPrompt()
{
    m_impl->purchasePromptTracker.MarkPromptDismissed(QDateTime::currentDateTimeUtc());
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
    m_impl->purchasePromptTracker.RecordSuccessfulRecreation();
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

void GuiController::ChangeMapMode(MapMode mode)
{
    emit mapModeChanged(mode);
}

bool GuiController::GetIsAdmin() const
{
    if constexpr (!Utils::IsMobile())
        return true;
    else
        return m_impl->isAdmin;
}

void GuiController::SetIsAdmin(bool value)
{
    m_impl->isAdmin = value;
    emit isAdminChanged(m_impl->isAdmin);
}
