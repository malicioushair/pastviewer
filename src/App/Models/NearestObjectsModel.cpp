#include "NearestObjectsModel.h"

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QGeoPositionInfoSource>
#include <QModelIndex>
#include <QVariant>

#include <algorithm>
#include <cassert>
#include <vector>

#include "App/Models/ScreenObjectsModel.h"
#include "glog/logging.h"

namespace {
constexpr auto MAX_NEAREST_ITEMS = 12;
}

struct NearestObjectsModel::Impl
{
	explicit Impl(ScreenObjectsModel * sourceModel, QGeoPositionInfoSource * positionSource)
		: sourceModel(sourceModel)
		, positionSource(positionSource)
		, currentPosition()
	{
	}

	ScreenObjectsModel * sourceModel;
	QGeoPositionInfoSource * positionSource;
	QGeoCoordinate currentPosition;

	// Map from proxy index to source model index
	std::vector<int> filteredIndices;
};

NearestObjectsModel::NearestObjectsModel(ScreenObjectsModel * sourceModel, QGeoPositionInfoSource * positionSource, QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>(sourceModel, positionSource))
{
	if (!sourceModel)
	{
		LOG(ERROR) << "NearestObjectsModel: sourceModel is null";
		return;
	}

	// Connect to source model changes
	connect(m_impl->sourceModel, &QAbstractListModel::rowsInserted, this, &NearestObjectsModel::OnSourceModelChanged);
	connect(m_impl->sourceModel, &QAbstractListModel::rowsRemoved, this, &NearestObjectsModel::OnSourceModelChanged);
	connect(m_impl->sourceModel, &QAbstractListModel::modelReset, this, &NearestObjectsModel::OnSourceModelChanged);
	connect(m_impl->sourceModel, &QAbstractListModel::dataChanged, this, [this](const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles) {
		// Forward data changes for items in the filtered list
		for (int sourceRow = topLeft.row(); sourceRow <= bottomRight.row(); ++sourceRow)
		{
			// Find if this source row is in our filtered list
			const auto it = std::find(m_impl->filteredIndices.begin(), m_impl->filteredIndices.end(), sourceRow);
			if (it != m_impl->filteredIndices.end())
			{
				const auto proxyRow = static_cast<int>(std::distance(m_impl->filteredIndices.begin(), it));
				const auto proxyIndex = index(proxyRow, 0);
				emit dataChanged(proxyIndex, proxyIndex, roles);
			}
		}
	});

	// Connect to position updates
	if (m_impl->positionSource)
		connect(m_impl->positionSource, &QGeoPositionInfoSource::positionUpdated, this, &NearestObjectsModel::OnPositionUpdated);

	// Connect count changes
	connect(this, &QAbstractListModel::rowsInserted, this, [this] { emit CountChanged(); });
	connect(this, &QAbstractListModel::rowsRemoved, this, [this] { emit CountChanged(); });
	connect(this, &QAbstractListModel::modelReset, this, [this] { emit CountChanged(); });

	// Initial update
	UpdateFilteredItems();
}

NearestObjectsModel::~NearestObjectsModel() = default;

int NearestObjectsModel::rowCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		return 0;
	return static_cast<int>(m_impl->filteredIndices.size());
}

QVariant NearestObjectsModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_impl->filteredIndices.size()))
		return QVariant();

	const auto sourceIndex = m_impl->filteredIndices[index.row()];
	const auto sourceModelIndex = m_impl->sourceModel->index(sourceIndex, 0);
	return m_impl->sourceModel->data(sourceModelIndex, role);
}

bool NearestObjectsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_impl->filteredIndices.size()))
		return false;

	const auto sourceIndex = m_impl->filteredIndices[index.row()];
	const auto sourceModelIndex = m_impl->sourceModel->index(sourceIndex, 0);
	const bool result = m_impl->sourceModel->setData(sourceModelIndex, value, role);

	if (result && role == Roles::Selected)
	{
		// Forward the dataChanged signal
		emit dataChanged(index, index, { role });
	}

	return result;
}

QHash<int, QByteArray> NearestObjectsModel::roleNames() const
{
	return m_impl->sourceModel->roleNames();
}

void NearestObjectsModel::OnPositionUpdated(const QGeoPositionInfo & info)
{
	const auto newPosition = info.coordinate();
	if (newPosition.isValid() && newPosition != m_impl->currentPosition)
	{
		m_impl->currentPosition = newPosition;
		UpdateFilteredItems();
	}
}

void NearestObjectsModel::OnSourceModelChanged()
{
	UpdateFilteredItems();
}

void NearestObjectsModel::UpdateFilteredItems()
{
	if (!m_impl->currentPosition.isValid() || m_impl->sourceModel->rowCount() == 0)
	{
		// No position or no source items, clear filtered list
		if (!m_impl->filteredIndices.empty())
		{
			beginResetModel();
			m_impl->filteredIndices.clear();
			endResetModel();
		}
		return;
	}

	// Create vector of (sourceIndex, distance) pairs
	std::vector<std::pair<int, double>> itemsWithDistance;
	const auto sourceRowCount = m_impl->sourceModel->rowCount();

	for (int i = 0; i < sourceRowCount; ++i)
	{
		const auto sourceIndex = m_impl->sourceModel->index(i, 0);
		const auto coord = m_impl->sourceModel->data(sourceIndex, Roles::Coordinate).value<QGeoCoordinate>();
		if (coord.isValid())
		{
			const auto distance = m_impl->currentPosition.distanceTo(coord);
			itemsWithDistance.emplace_back(i, distance);
		}
	}

	// Sort by distance (nearest first)
	std::sort(itemsWithDistance.begin(), itemsWithDistance.end(), [](const auto & a, const auto & b) {
		return a.second < b.second;
	});

	// Take only the nearest MAX_NEAREST_ITEMS
	const auto newSize = std::min(static_cast<size_t>(MAX_NEAREST_ITEMS), itemsWithDistance.size());
	std::vector<int> newFilteredIndices;
	newFilteredIndices.reserve(newSize);
	for (size_t i = 0; i < newSize; ++i)
		newFilteredIndices.push_back(itemsWithDistance[i].first);

	// Check if the filtered list changed
	const bool indicesChanged = newFilteredIndices != m_impl->filteredIndices;

	if (indicesChanged)
	{
		beginResetModel();
		m_impl->filteredIndices = std::move(newFilteredIndices);
		endResetModel();
	}
}
