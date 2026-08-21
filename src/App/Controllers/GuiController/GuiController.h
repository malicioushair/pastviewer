#pragma once

#include <memory>

#include <QObject>
#include <QPermission>
#include <QQmlEngine>
#include <QQuickItemGrabResult>
#include <qqmlintegration.h>

namespace PastViewer {
class GuiController
	: public QObject
{
	Q_OBJECT
	QML_ELEMENT
	QML_SINGLETON
	Q_DISABLE_COPY(GuiController)

signals:
	void PermissionGranted(const QPermission & permission);
	void showErrorDialog(const QString & errorMessage);
	void onboardingReset();
	void tipsPromptRequested();

public:
	static std::unique_ptr<GuiController> Create();
	static GuiController * create(QQmlEngine * qmlEngine, QJSEngine * jsEngine);

	~GuiController();

	Q_INVOKABLE bool IsDebug();
	Q_INVOKABLE void BumpHotReloadToken();
	Q_INVOKABLE QString GetAppVersion();
	Q_INVOKABLE bool ShouldShowChangelog() const;
	Q_INVOKABLE void MarkChangelogShown();
	Q_INVOKABLE bool HasTipsUrl() const;
	Q_INVOKABLE bool OpenTipsUrl();
	Q_INVOKABLE bool ShouldShowTipsPrompt();
	Q_INVOKABLE void NotifyCameraModeLeft();
	Q_INVOKABLE void MarkTipsPromptShown();
	Q_INVOKABLE void DismissTipsPrompt();
	Q_INVOKABLE void RequestCameraPermission();
	Q_INVOKABLE bool SaveScreenshotToGallery(const QString & filePath);
	Q_INVOKABLE QString SaveImage(const QQuickItemGrabResult * grabResult);
	Q_INVOKABLE bool ShareImage();

	Q_INVOKABLE bool IsOnboardingStepCompleted(const QString & key);
	Q_INVOKABLE void SetOnboardingStepCompleted(const QString & key);
	Q_INVOKABLE void ResetOnboarding();

private:
	// hiding the constructor from QML so it will have to use the static create(QQmlEngine * qmlEngine, QJSEngine * jsEngine)
	GuiController();

private:
	void RequestPermission(const QPermission & permission);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace PastViewer
