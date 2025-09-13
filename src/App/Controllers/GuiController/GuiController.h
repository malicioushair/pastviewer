#pragma once

#include <QObject>
#include <QQmlApplicationEngine>

namespace TorrentPlayer {
class GuiController
	: public QObject
{
	Q_OBJECT
	Q_DISABLE_COPY(GuiController)

public:
	GuiController(QObject * parent = nullptr);
	~GuiController();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
} // namespace TorrentPlayer