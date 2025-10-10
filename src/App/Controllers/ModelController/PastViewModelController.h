#pragma once

#include <QAbstractItemModel>
#include <QObject>

#include <memory>

class PastVuModelController
	: public QObject
{
	Q_OBJECT

public:
	PastVuModelController(QObject * parent = nullptr);
	~PastVuModelController();

	Q_INVOKABLE QAbstractListModel * GetModel();
	Q_INVOKABLE QString GetMapHostApiKey();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
