#pragma once

#include <memory>

#include <QObject>
#include <QPermission>
#include <QQmlEngine>
#include <QQuickItemGrabResult>
#include <QVariantList>

#include "App/Utils/NonCopyMovable.h"

namespace PastViewer {
class GuiController
	: public QObject
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON
    NON_COPY_MOVABLE(GuiController)

    Q_PROPERTY(QVariantList tipProducts READ TipProducts NOTIFY tipProductsChanged)
    Q_PROPERTY(bool tipProductsLoading READ TipProductsLoading NOTIFY tipProductsLoadingChanged)
    Q_PROPERTY(bool tipPurchaseInProgress READ TipPurchaseInProgress NOTIFY tipPurchaseInProgressChanged)
    Q_PROPERTY(QVariant currentTour READ GetCurrentTour WRITE SetCurrentTour NOTIFY CurrentTourChanged) // @todo think on moving to a MapController

public:
    enum class MapMode
    {
        Main,
        TourCreation,
        DisplayTour,
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
    void CurrentTourChanged();

    void showErrorDialog(const QString & errorMessage);
    void onboardingReset();
    void tipsPromptRequested();
	void tipJarRequested();
	void tipProductsChanged();
	void tipProductsLoadingChanged();
	void tipPurchaseInProgressChanged();
	void tipPurchaseSucceeded();
	void tipPurchasePending();
	void tipOperationFailed();
	void mapModeChanged(MapMode mode);

public:
	static std::unique_ptr<GuiController> Create();
	static GuiController * create(QQmlEngine * qmlEngine, QJSEngine * jsEngine);

	~GuiController();

	Q_INVOKABLE bool IsDebug();
	Q_INVOKABLE void BumpHotReloadToken();
	Q_INVOKABLE QString GetAppVersion();
	Q_INVOKABLE bool ShouldShowChangelog() const;
	Q_INVOKABLE void MarkChangelogShown();
	Q_INVOKABLE bool HasTipSupport() const;
	Q_INVOKABLE void RequestTipFlow();
	Q_INVOKABLE void LoadTipProducts();
	Q_INVOKABLE void PurchaseTip(const QString & productId);
	Q_INVOKABLE bool ShouldShowTipsPrompt();
	Q_INVOKABLE void NotifyCameraModeLeft();
	Q_INVOKABLE void MarkTipsPromptShown();
	Q_INVOKABLE void DismissTipsPrompt();
	Q_INVOKABLE void RequestCameraPermission();
	Q_INVOKABLE bool SaveScreenshotToGallery(const QString & filePath);
	Q_INVOKABLE QString SaveImage(const QQuickItemGrabResult * grabResult);
	Q_INVOKABLE bool ShareImage();
	Q_INVOKABLE void ChangeMapMode(MapMode mode);
	QVariantList TipProducts() const;
	bool TipProductsLoading() const;
	bool TipPurchaseInProgress() const;

	Q_INVOKABLE bool IsOnboardingStepCompleted(const QString & key);
	Q_INVOKABLE void SetOnboardingStepCompleted(const QString & key);
	Q_INVOKABLE void ResetOnboarding();

private:
	// hiding the constructor from QML so it will have to use the static create(QQmlEngine * qmlEngine, QJSEngine * jsEngine)
	GuiController();

private:
	void RequestPermission(const QPermission & permission);
    QVariant GetCurrentTour() const;
    void SetCurrentTour(const QVariant & tour);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace PastViewer
