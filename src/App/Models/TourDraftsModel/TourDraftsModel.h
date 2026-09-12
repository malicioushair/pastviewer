#pragma once

#include "App/Tours/Tours.h"
#include <QAbstractListModel>

#include <QtCore/qvariant.h>
#include <memory>

class TourDraftsModel
	: public QAbstractListModel
{
public:
	enum Roles
	{
		Title = Qt::UserRole + 1,
		Description,
		Delete,
	};

	explicit TourDraftsModel(QObject * parent = nullptr);
	~TourDraftsModel();

public: // QAbstractListModel
	int rowCount(const QModelIndex & parent = {}) const override;
	QVariant data(const QModelIndex & index, int role) const override;
	bool setData(const QModelIndex & index, const QVariant & value, int role) override;
	QHash<int, QByteArray> roleNames() const override;

public:
	void Update();
	std::vector<Tours::Tour> GetTourDrafts() const;
	void AddTour(const Tours::Tour & tour);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};