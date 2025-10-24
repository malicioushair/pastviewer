#pragma once

#include <QObject>
#include <QQmlApplicationEngine>

namespace PastViewer {
class GuiController
	: public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(GuiController)

signals:
	void PositionPermissionGranted();

public:
	GuiController(QObject * parent = nullptr);
	~GuiController();

	Q_INVOKABLE bool IsDebug();
	Q_INVOKABLE void BumpHotReloadToken();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace PastViewer