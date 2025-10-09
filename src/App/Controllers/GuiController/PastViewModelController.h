#pragma once

#include <QObject>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qtmetamacros.h>
#include <memory>

class PastVuModelController
	: public QObject
{
	Q_OBJECT

public:
	PastVuModelController(QObject * parent = nullptr);
	~PastVuModelController();

	Q_INVOKABLE QAbstractListModel * GetModel();
	Q_INVOKABLE std::string GetMapHostApiKey();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
