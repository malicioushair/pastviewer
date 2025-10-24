#pragma once

#include <QAbstractItemModel>
#include <QObject>

#include <memory>

class PastVuModelController
	: public QObject
{
	Q_OBJECT

signals:
	void PositionPermissionGranted();

public:
	PastVuModelController(QObject * parent = nullptr);
	~PastVuModelController();

	Q_INVOKABLE QAbstractListModel * GetModel();
	Q_INVOKABLE QString GetMapHostApiKey();

	void OnPositionPermissionGranted();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
