#include "TourDraftsModel.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>

#include <QDir>
#include <QJSonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <vector>

#include "App/Tours/Tours.h"
#include "App/Utils/JsonHelpers.h"

#include "glog/logging.h"

struct TourDraftsModel::Impl
{
	std::vector<Tours::Tour> tours {};
};

TourDraftsModel::TourDraftsModel(QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>())
{
	Update();
}

TourDraftsModel::~TourDraftsModel() = default;

int TourDraftsModel::rowCount(const QModelIndex &) const
{
	return static_cast<int>(m_impl->tours.size());
}

QVariant TourDraftsModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};
	const auto item = m_impl->tours.at(index.row());
	switch (role)
	{
		case Title:
			return item.title;
		case Description:
			return item.descrption;
		default:
			assert(false && "Unknown role");
	}
	return {};
}

bool TourDraftsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (!index.isValid())
		return assert(false && "invalid index"), false;

	const auto item = m_impl->tours.at(index.row());
	switch (role)
	{
		case Delete:
		{
			const auto filePath = Tours::GetDraftFileLocation(item.title);

			if (QFile file(filePath); !file.remove())
			{
				LOG(WARNING) << std::format("Could not remove the file: {} with error: {}",
					filePath.toStdString(),
					file.errorString().toStdString());
				return false;
			}

			beginRemoveRows({}, index.row(), index.row());
			const auto removed = std::ranges::remove_if(m_impl->tours, [&](decltype(m_impl->tours)::const_reference tour) {
				return tour.title == item.title;
			});
			m_impl->tours.erase(removed.begin(), removed.end());
			endRemoveRows();

			return true;
		}
		default:
			assert(false && "unknown role");
	}
	return false;
}

QHash<int, QByteArray> TourDraftsModel::roleNames() const
{
#define ROLENAME(NAME)     \
	{                      \
		Roles::NAME, #NAME \
	}
	return {
		ROLENAME(Title),
		ROLENAME(Description),
		ROLENAME(Delete),
	};
#undef ROLENAME
}

void TourDraftsModel::Update()
{
	QJsonDocument doc;
	const auto dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/draft_tours";
	for (const auto & dirEntry : QDirListing(dirPath))
	{
		const auto json = JsonHelpers::ReadJson(dirEntry.absoluteFilePath());
		Tours::Tour tour {
			.id = json["id"].toInt(),
			.title = json["title"].toString(),
			.descrption = json["description"].toString(),
			.isDraft = json["isDraft"].isBool(),
		};
		const auto stops = json["stops"].toArray();
		std::ranges::transform(stops, std::back_inserter(tour.stops), [](decltype(stops)::const_reference value) {
			return Tours::TourStop {
				.id = value["id"].toInt(),
				.title = value["title"].toString(),
				.description = value["description"].toString(),
				.audioFile = value["audioFile"].toString(),
				.imageFile = value["imageFile"].toString(),
			};
		});
		if (std::ranges::none_of(m_impl->tours, [&](const Tours::Tour & item) { return item.id == tour.id; }))
		{
			beginInsertRows({}, 0, rowCount());
			m_impl->tours.push_back(tour);
			endInsertRows();
		}
	}
}

std::vector<Tours::Tour> TourDraftsModel::GetTourDrafts() const
{
	return m_impl->tours;
}

void TourDraftsModel::AddTour(const Tours::Tour & tour)
{
	beginInsertRows({}, 0, rowCount());
	m_impl->tours.emplace_back(tour);
	endInsertRows();
}
