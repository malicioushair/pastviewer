#pragma once

#include <QObject>
#include <QPermission>
#include <QQmlApplicationEngine>
#include <QQuickItemGrabResult>
#include <QVariantList>

#include "App/Utils/NonCopyMovable.h"

namespace PastViewer {
class Analytics;

class GuiController
    : public QObject
{
    Q_OBJECT
    NON_COPY_MOVABLE(GuiController)

    Q_PROPERTY(bool isAdmin READ GetIsAdmin WRITE SetIsAdmin NOTIFY isAdminChanged)

public:
    enum class MapMode
    {
        Main,
        TourCreation,
        DisplayTour,
        PlaceTour,
    };
    Q_ENUM(MapMode)

    enum class TourInfoMode
    {
        Info,
        Edit,
        Tour,
    };
    Q_ENUM(TourInfoMode)

signals:
    void PermissionGranted(const QPermission & permission);

    void showErrorDialog(const QString & errorMessage);
    void onboardingReset();
    void tipsPromptRequested();
    void mapModeChanged(MapMode mode);
    void isAdminChanged(bool value);
    void locationDisclosureRequested();

public:
    explicit GuiController(QObject * parent = nullptr);

    ~GuiController();

    Q_INVOKABLE bool IsDebug();
    Q_INVOKABLE void BumpHotReloadToken();
    Q_INVOKABLE QString GetAppVersion();
    Q_INVOKABLE QString GetPrivacyPolicy() const;
    Q_INVOKABLE bool ShouldShowChangelog() const;
    Q_INVOKABLE void MarkChangelogShown();
    Q_INVOKABLE bool ShouldShowTipsPrompt();
    Q_INVOKABLE void NotifyCameraModeLeft();
    Q_INVOKABLE void MarkTipsPromptShown();
    Q_INVOKABLE void DismissTipsPrompt();
    Q_INVOKABLE void RequestCameraPermission();
    Q_INVOKABLE void RespondToLocationDisclosure(bool accepted);
    Q_INVOKABLE bool SaveScreenshotToGallery(const QString & filePath);
    Q_INVOKABLE QString SaveImage(const QQuickItemGrabResult * grabResult);
    Q_INVOKABLE bool ShareImage();
    Q_INVOKABLE void ChangeMapMode(MapMode mode);

    Q_INVOKABLE bool IsOnboardingStepCompleted(const QString & key);
    Q_INVOKABLE void SetOnboardingStepCompleted(const QString & key);
    Q_INVOKABLE void ResetOnboarding();

private:
    void RequestPermission(const QPermission & permission);
    bool GetIsAdmin() const;
    void SetIsAdmin(bool value);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace PastViewer
