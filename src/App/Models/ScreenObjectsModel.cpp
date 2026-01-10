#include "ScreenObjectsModel.h"

#include <QAbstractListModel>
#include <QGeoPositionInfoSource>
#include <QGeoRectangle>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>

#include <cassert>
#include <memory>

#include "glog/logging.h"

#include "App/Models/BaseModel.h"
#include "App/Utils/Range.h"

struct ScreenObjectsModel::Impl
{
	std::unordered_set<int> belongsToTimeline;
	QHash<int, int> cidToZoomToDecluster;
	QSettings settings;
	Range timeline {
		settings.value("YEARS_FROM", 1800).toInt(),
		settings.value("YEARS_TO", QDate::currentDate().year()).toInt()
	};
};

ScreenObjectsModel::ScreenObjectsModel(QAbstractListModel * sourceModel, QObject * parent)
	: QSortFilterProxyModel(parent)
	, m_impl(std::make_unique<Impl>())
{
	if (assert(sourceModel); !sourceModel)
	{
		LOG(ERROR) << "NearestObjectsModel: sourceModel is null";
		return;
	}

	setSourceModel(sourceModel);

	connect(sourceModel, &QAbstractListModel::rowsInserted, this, &ScreenObjectsModel::OnSourceModelChanged);
	connect(sourceModel, &QAbstractListModel::rowsRemoved, this, &ScreenObjectsModel::OnSourceModelChanged);
	connect(sourceModel, &QAbstractListModel::modelReset, this, &ScreenObjectsModel::OnSourceModelChanged);

	connect(this, &QSortFilterProxyModel::rowsInserted, this, [this] { emit CountChanged(); });
	connect(this, &QSortFilterProxyModel::rowsRemoved, this, [this] { emit CountChanged(); });
	connect(this, &QSortFilterProxyModel::modelReset, this, [this] { emit CountChanged(); });

	UpdateAcceptedRows();
	invalidateFilter();
}

ScreenObjectsModel::~ScreenObjectsModel() = default;

void ScreenObjectsModel::OnUserSelectedTimelineRangeChanged(const Range & timeline)
{
	m_impl->timeline = timeline;
	UpdateAcceptedRows();
	invalidateFilter();
}

void ScreenObjectsModel::UpdateZoomsToDecluster(const QHash<int, int> & cidsToZooms)
{
	m_impl->cidToZoomToDecluster = cidsToZooms;
}

QVariant ScreenObjectsModel::data(const QModelIndex & index, int role) const
{
	const auto sourceIndex = mapToSource(index);
	switch (role)
	{
		case IsClustered:
		{
			const auto cid = sourceModel()->data(sourceIndex, BaseModel::Roles::Cid).toInt();
			return m_impl->cidToZoomToDecluster.contains(cid);
		}
		case ZoomToDecluster:
		{
			const auto cid = sourceModel()->data(sourceIndex, BaseModel::Roles::Cid).toInt();
			const auto it = m_impl->cidToZoomToDecluster.find(cid);
			return it == m_impl->cidToZoomToDecluster.end() ? 0 : it.value();
		}
	}
	return sourceModel()->data(sourceIndex, role);
}

QHash<int, QByteArray> ScreenObjectsModel::roleNames() const
{
	auto roles = sourceModel()->roleNames();
#define ROLENAME(NAME) roles[NAME] = #NAME
	ROLENAME(IsClustered);
	ROLENAME(ZoomToDecluster);
#undef ROLENAME
	return roles;
}

bool ScreenObjectsModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
	if (source_parent.isValid())
		return false;

	return m_impl->belongsToTimeline.find(source_row) != m_impl->belongsToTimeline.cend();
}

void ScreenObjectsModel::OnPositionUpdated(const QGeoPositionInfo & info)
{
}

void ScreenObjectsModel::OnSourceModelChanged()
{
	UpdateAcceptedRows();
	invalidateFilter();
}

void ScreenObjectsModel::UpdateAcceptedRows()
{
	m_impl->belongsToTimeline.clear();

	const auto * sourceModel = this->sourceModel();
	if (!sourceModel || sourceModel->rowCount() == 0)
		return;

	const auto sourceRowCount = sourceModel->rowCount();

	for (int i = 0; i < sourceRowCount; ++i)
	{
		const auto sourceIndex = sourceModel->index(i, 0);
		const auto year = sourceModel->data(sourceIndex, BaseModel::Roles::Year).toInt();

		if (year > m_impl->timeline.min && year <= m_impl->timeline.max)
			m_impl->belongsToTimeline.insert(i);
	}
}
