#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QtCore/qtmetamacros.h>

namespace PastViewer {
class GuiController
	: public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(GuiController)

signals:
	void PositionPermissionGranted();
	void showErrorDialog(const QString & errorMessage);

public:
	GuiController(QObject * parent = nullptr);
	~GuiController();

	Q_INVOKABLE bool IsDebug();
	Q_INVOKABLE void BumpHotReloadToken();
	Q_INVOKABLE QString GetAppVersion();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace PastViewer