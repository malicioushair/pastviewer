#include "AvailableToursModel.h"

#include <cassert>
#include <format>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#include "glog/logging.h"

namespace {
constexpr auto URL = "https://pastviewer.com/api/v1/tours";

struct Item
{
	QString title {};
	QString description {};
	QString imageUrl {};
};

}

struct AvailableToursModel::Impl
{
	QNetworkAccessManager networkManager {};
	std::vector<Item> items;
};

AvailableToursModel::AvailableToursModel(QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>())
{
	connect(&m_impl->networkManager, &QNetworkAccessManager::finished, this, &AvailableToursModel::OnNetworkReplyFinished);
	Update();
}

AvailableToursModel::~AvailableToursModel() = default;

void AvailableToursModel::OnNetworkReplyFinished(QNetworkReply * reply)
{
	if (reply->error())
	{
		LOG(ERROR) << std::format("Error occured when requested tour list: {}", reply->errorString().toStdString());
		reply->deleteLater();
		return;
	}

	LOG(INFO) << "Reply success";
	const auto body = reply->readAll();
	reply->deleteLater();

	QJsonParseError parserError;
	const auto jsonDoc = QJsonDocument::fromJson(body, &parserError);
	if (parserError.error != QJsonParseError::NoError)
	{
		LOG(WARNING) << "Failed to parse JSON with error" << parserError.errorString().toStdString();
		return;
	}

	const auto root = jsonDoc.object();
	const auto items = root.value("items").toArray();
	// The model is yet small, we can afford to reload it for now
	m_impl->items.clear();
	for (const auto & item : items)
	{
		const auto itemObj = item.toObject();
		beginResetModel();
		m_impl->items.push_back({
			.title = itemObj.value("title").toString(),
			.description = itemObj.value("description").toString(),
			.imageUrl = itemObj.value("imageUrl").toString(),
		});
		endResetModel();
	}
}

void AvailableToursModel::Update()
{
	QNetworkRequest request({ URL });
	m_impl->networkManager.get(request);
}

int AvailableToursModel::rowCount(const QModelIndex & parent) const
{
	return static_cast<int>(m_impl->items.size());
}

QVariant AvailableToursModel::data(const QModelIndex & index, int role) const
{
	assert(index.isValid());

	const auto item = m_impl->items.at(index.row());
	switch (role)
	{
		case Title:
			return item.title;
		case Description:
			return item.description;
		case ImageUrl:
			return item.imageUrl;
		default:
			assert(false && "unknown role");
	}
	return {};
}

QHash<int, QByteArray> AvailableToursModel::roleNames() const
{
#define ROLENAME(NAME)     \
	{                      \
		Roles::NAME, #NAME \
	}
	return {
		ROLENAME(Title),
		ROLENAME(Description),
		ROLENAME(ImageUrl),
	};
#undef ROLENAME
}
