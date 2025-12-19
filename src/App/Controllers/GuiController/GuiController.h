#pragma once

#include <QObject>
#include <QPermission>
#include <QQmlApplicationEngine>
#include <QQuickItemGrabResult>

namespace PastViewer {
class GuiController
	: public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(GuiController)

signals:
	void PermissionGranted(const QPermission & permission);
	void showErrorDialog(const QString & errorMessage);

public:
	GuiController(QObject * parent = nullptr);
	~GuiController();

	Q_INVOKABLE bool IsDebug();
	Q_INVOKABLE void BumpHotReloadToken();
	Q_INVOKABLE QString GetAppVersion();
	Q_INVOKABLE void RequestCameraPermission();
	Q_INVOKABLE bool SaveScreenshotToGallery(const QString & filePath);
	Q_INVOKABLE void SaveImage(const QQuickItemGrabResult * grabResult);

private:
	void RequestPermission(const QPermission & permission);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace PastViewer