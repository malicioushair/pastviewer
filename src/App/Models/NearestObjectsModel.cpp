#include "NearestObjectsModel.h"

#include <algorithm>
#include <cmath>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QMetaObject>
#include <QModelIndex>
#include <QVariant>

#include <unordered_set>

#include "App/Models/BaseModel.h"

#include "glog/logging.h"

namespace {
constexpr auto MAX_DISTANCE_METERS = 4000.0;
}

struct NearestObjectsModel::Impl
{
	explicit Impl(QGeoPositionInfoSource * positionSource)
		: positionSource(positionSource)
		, currentPosition()
	{
	}

	QGeoPositionInfoSource * positionSource;
	QGeoCoordinate currentPosition;

	std::unordered_set<int> withinDistanceIndices;
};

NearestObjectsModel::NearestObjectsModel(QAbstractItemModel * sourceModel, QGeoPositionInfoSource * positionSource, QObject * parent)
	: QSortFilterProxyModel(parent)
	, m_impl(std::make_unique<Impl>(positionSource))
{
	if (assert(sourceModel); !sourceModel)
	{
		LOG(ERROR) << "NearestObjectsModel: sourceModel is null";
		return;
	}

	setSourceModel(sourceModel);
	setDynamicSortFilter(false);

	connect(sourceModel, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &, const QModelIndex &, const QVector<int> & roles) {
		const bool onlySelectedRole = !roles.isEmpty()
								   && std::all_of(roles.cbegin(), roles.cend(), [](int role) {
										return role == BaseModel::Roles::Selected;
									});
		if (!onlySelectedRole)
			OnSourceModelChanged();
	});
	connect(sourceModel, &QAbstractListModel::rowsInserted, this, &NearestObjectsModel::OnSourceModelChanged);
	connect(sourceModel, &QAbstractListModel::rowsRemoved, this, &NearestObjectsModel::OnSourceModelChanged);
	connect(sourceModel, &QAbstractListModel::modelReset, this, &NearestObjectsModel::OnSourceModelChanged);

	if (m_impl->positionSource)
		connect(m_impl->positionSource, &QGeoPositionInfoSource::positionUpdated, this, &NearestObjectsModel::OnPositionUpdated);

	connect(this, &QSortFilterProxyModel::rowsInserted, this, [this] { emit CountChanged(); });
	connect(this, &QSortFilterProxyModel::rowsRemoved, this, [this] { emit CountChanged(); });
	connect(this, &QSortFilterProxyModel::modelReset, this, [this] { emit CountChanged(); });

	OnSourceModelChanged();
}

NearestObjectsModel::~NearestObjectsModel() = default;

bool NearestObjectsModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{
	if (source_parent.isValid())
		return false;

	return m_impl->withinDistanceIndices.find(source_row) != m_impl->withinDistanceIndices.cend();
}

void NearestObjectsModel::OnPositionUpdated(const QGeoPositionInfo & info)
{
	const auto newPosition = info.coordinate();
	if (newPosition.isValid() && newPosition != m_impl->currentPosition)
	{
		m_impl->currentPosition = newPosition;
		OnSourceModelChanged();
	}
}

void NearestObjectsModel::OnSourceModelChanged()
{
	RebuildProxyModel();
}

void NearestObjectsModel::RebuildProxyModel()
{
	UpdateAcceptedRows();
	invalidateFilter();
}

void NearestObjectsModel::UpdateAcceptedRows()
{
	m_impl->withinDistanceIndices.clear();

	if (!m_impl->currentPosition.isValid())
		return;

	const auto * sourceModel = this->sourceModel();
	if (!sourceModel || sourceModel->rowCount() == 0)
		return;

	const auto sourceRowCount = sourceModel->rowCount();

	for (int i = 0; i < sourceRowCount; ++i)
	{
		const auto sourceIndex = sourceModel->index(i, 0);
		const auto coord = sourceModel->data(sourceIndex, BaseModel::Roles::Coordinate).value<QGeoCoordinate>();
		if (coord.isValid())
		{
			const auto distance = m_impl->currentPosition.distanceTo(coord);
			if (!std::isfinite(distance))
				continue;

			if (distance <= MAX_DISTANCE_METERS)
				m_impl->withinDistanceIndices.insert(i);
		}
	}
}
