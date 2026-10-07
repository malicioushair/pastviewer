#pragma once

#include <memory>

#include <QAbstractListModel>

class QNetworkReply;

class AvailableToursModel
	: public QAbstractListModel
{
	Q_OBJECT

public:
	enum Roles
	{
		Title = Qt::UserRole + 1,
		Description,
		ImageUrl,
	};

	explicit AvailableToursModel(QObject * parent = nullptr);
	~AvailableToursModel();

public: // QAbstractListModel
	int rowCount(const QModelIndex & parent = {}) const override;
	QVariant data(const QModelIndex & index, int role) const override;
	QHash<int, QByteArray> roleNames() const override;

private:
	void OnNetworkReplyFinished(QNetworkReply * reply);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};