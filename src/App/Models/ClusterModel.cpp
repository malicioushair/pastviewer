#include "ClusterModel.h"

#include "App/Models/BaseModel.h"

#include <QPointF>
#include <QtCore/qpoint.h>
#include <QtPositioning/qgeocoordinate.h>
#include <cmath>
#include <numbers>
#include <queue>
#include <unordered_map>
#include <variant>
#include <vector>

namespace {

double ClampLat(double lat)
{
	constexpr double maxLat = 85.05112878;
	if (lat > maxLat)
		return maxLat;
	if (lat < -maxLat)
		return -maxLat;
	return lat;
}

double WrapLon(double lon)
{
	double x = std::fmod(lon + 180.0, 360.0);
	if (x < 0)
		x += 360.0;
	return x - 180.0; // [-180, 180)
}

double WorldSizeForZoom(int zoom)
{
	return std::ldexp(256.0, zoom);
}

QPointF GeoToWorldPx(const QGeoCoordinate & c, int z)
{
	const auto world = WorldSizeForZoom(z);

	const auto lat = ClampLat(c.latitude());
	const auto lon = WrapLon(c.longitude());

	const auto x = (lon + 180.0) / 360.0 * world;

	const auto latRad = qDegreesToRadians(lat);
	const auto sinLat = std::sin(latRad);

	const auto y = (0.5 - std::log((1.0 + sinLat) / (1.0 - sinLat)) / (4.0 * std::numbers::pi)) * world;

	return { x, y };
}

double AlignWrappedX(double x, double refX, double worldSize)
{
	while (x < refX - worldSize / 2.0)
		x += worldSize;
	while (x > refX + worldSize / 2.0)
		x -= worldSize;
	return x;
}

QPointF GeoToScreenCoords(const QGeoRectangle & viewport, int zoomLevel, const QGeoCoordinate & geoCoord)
{
	const auto worldSize = WorldSizeForZoom(zoomLevel);

	auto topLeft = GeoToWorldPx(viewport.topLeft(), zoomLevel);
	auto currentPoint = GeoToWorldPx(geoCoord, zoomLevel);

	currentPoint.setX(AlignWrappedX(currentPoint.x(), topLeft.x(), worldSize));

	return { currentPoint.x() - topLeft.x(), currentPoint.y() - topLeft.y() };
}

}

struct ClusterModel::Impl
{
	const QAbstractItemModel & sourceModel;
	std::vector<Node> m_nodes;
	QGeoRectangle viewport;
};

ClusterModel::ClusterModel(const QAbstractItemModel & sourceModel, QObject * parent)
	: QAbstractListModel()
	, m_impl(std::make_unique<Impl>(sourceModel))

{
	connect(&m_impl->sourceModel, &QAbstractItemModel::modelReset, this, &ClusterModel::modelReset);
	connect(&m_impl->sourceModel, &QAbstractItemModel::rowsInserted, this, &ClusterModel::rowsInserted);
	connect(&m_impl->sourceModel, &QAbstractItemModel::rowsRemoved, this, &ClusterModel::rowsRemoved);
	connect(&m_impl->sourceModel, &QAbstractItemModel::dataChanged, this, &ClusterModel::dataChanged);
}

ClusterModel::~ClusterModel() = default;

int ClusterModel::rowCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		return 0;
	return static_cast<int>(m_impl->m_nodes.size());
}

QVariant ClusterModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_impl->m_nodes.size()))
		return QVariant();

	const auto & node = m_impl->m_nodes[index.row()];

	// Handle cluster-specific roles
	if (role == IsCluster)
		return std::holds_alternative<ClusterNode>(node);

	if (role == ClusterCount)
	{
		if (std::holds_alternative<ClusterNode>(node))
		{
			const auto & clusterNode = std::get<ClusterNode>(node);
			return clusterNode.members.size();
		}
		return 1; // Individual node has count of 1
	}

	if (std::holds_alternative<IndividualNode>(node))
	{
		const auto & individualNode = std::get<IndividualNode>(node);
		if (!individualNode.src.isValid())
			return QVariant();
		return m_impl->sourceModel.data(individualNode.src, role);
	}
	else if (std::holds_alternative<ClusterNode>(node))
	{
		const auto & clusterNode = std::get<ClusterNode>(node);

		// For clusters, return centroid coordinate and aggregate data from first member
		if (role == BaseModel::Coordinate)
			return QVariant::fromValue(clusterNode.centroid);

		// For other roles, use data from the first member
		if (!clusterNode.members.isEmpty() && clusterNode.members[0].isValid())
			return m_impl->sourceModel.data(clusterNode.members[0], role);
	}

	return QVariant();
}

QHash<int, QByteArray> ClusterModel::roleNames() const
{
	auto roles = m_impl->sourceModel.roleNames();
	roles[IsCluster] = "IsCluster";
	roles[ClusterCount] = "ClusterCount";
	return roles;
}

std::vector<Node> ClusterModel::BuildClusters() const
{
	std::vector<Node> nodes;

	constexpr double CLUSTER_THRESHOLD = 20.0; // pixels
	constexpr double CLUSTER_THRESHOLD_SQ = CLUSTER_THRESHOLD * CLUSTER_THRESHOLD;
	constexpr double CELL_SIZE = CLUSTER_THRESHOLD; // Use threshold as cell size

	struct Item
	{
		int srcIndex; // mb cid?
		QPersistentModelIndex sourceIndex;
		QGeoCoordinate geoCoord;
		QPointF screenPos;
		int cellX;
		int cellY;
	};

	// Step 1: Build list items[]
	std::vector<Item> items;
	items.reserve(m_impl->sourceModel.rowCount());

	for (int i = 0, sz = m_impl->sourceModel.rowCount(); i < sz; ++i)
	{
		const auto sourceIndex = m_impl->sourceModel.index(i, 0);
		// auto itemData = m_impl->sourceModel.itemData(sourceIndex);
		const auto coords = m_impl->sourceModel.data(sourceIndex, BaseModel::Coordinate).value<QGeoCoordinate>();
		const auto zoomLevel = m_impl->sourceModel.data(sourceIndex, BaseModel::ZoomLevel).toInt();
		const auto screenCoords = GeoToScreenCoords(m_impl->viewport, zoomLevel, coords);

		const auto cellX = static_cast<int>(std::floor(screenCoords.x() / CELL_SIZE));
		const auto cellY = static_cast<int>(std::floor(screenCoords.y() / CELL_SIZE));

		// clang-format off
		items.push_back({
            .srcIndex = i,
			.sourceIndex = QPersistentModelIndex(sourceIndex),
			.geoCoord = coords,
			.screenPos = screenCoords,
			.cellX = cellX,
			.cellY = cellY
        });
		// clang-format on
	}

	if (items.empty())
		return nodes; // mb just return {} and do not create nodes beforehand ?

	// Step 2: Build grid map cell -> [item indices]
	using CellKey = std::pair<int, int>; // {x, y}

	struct CellKeyHash // use lambda instead?
	{
		std::size_t operator()(const CellKey & key) const
		{
			return std::hash<int>()(key.first) ^ (std::hash<int>()(key.second) << 1); // does it have to be that complecated? why not just use std::hash?
		}
	};

	std::unordered_map<CellKey, std::vector<int>, CellKeyHash> gridMap;

	for (size_t i = 0; i < items.size(); ++i)
	{
		CellKey key { items[i].cellX, items[i].cellY };
		gridMap[key].push_back(static_cast<int>(i));
	}

	// Step 3: Visited array
	std::vector<bool> visited(items.size(), false);

	// Step 4: For each item not visited, start clustering
	for (size_t i = 0; i < items.size(); ++i)
	{
		if (visited[i])
			continue;

		// Start a new cluster
		std::queue<int> queue;
		std::vector<int> members; // reserve ?

		queue.push(static_cast<int>(i)); // do we realy need the queue?
		members.push_back(static_cast<int>(i));
		visited[i] = true;

		// BFS: while queue not empty
		while (!queue.empty())
		{
			const int current = queue.front();
			queue.pop();

			const auto & currentItem = items[current];
			const int centerCellX = currentItem.cellX;
			const int centerCellY = currentItem.cellY;

			// Look at buckets in neighbor cells (9 cells: 3x3 grid)
			for (int dx = -1; dx <= 1; ++dx)
			{
				for (int dy = -1; dy <= 1; ++dy)
				{
					const CellKey neighborKey { centerCellX + dx, centerCellY + dy };
					const auto it = gridMap.find(neighborKey);
					if (it == gridMap.end())
						continue;

					// For each candidate in this cell
					const auto [_, itemIndices] = *it;
					for (int candidateIdx : itemIndices)
					{
						if (visited[candidateIdx])
							continue;

						const auto & candidate = items[candidateIdx];
						const double dx_px = candidate.screenPos.x() - currentItem.screenPos.x();
						const double dy_px = candidate.screenPos.y() - currentItem.screenPos.y();
						const double distSq = dx_px * dx_px + dy_px * dy_px;

						// If distance² <= threshold²
						if (distSq <= CLUSTER_THRESHOLD_SQ)
						{
							visited[candidateIdx] = true;
							queue.push(candidateIdx);
							members.push_back(candidateIdx);
						}
					}
				}
			}
		}

		// Output cluster or individual
		if (members.size() == 1)
		{
			// Output individual
			nodes.push_back(IndividualNode { items[members[0]].sourceIndex });
		}
		else
		{
			// Output cluster with centroid
			double sumLat = 0.0;
			double sumLon = 0.0;
			QVector<QPersistentModelIndex> memberIndices;
			memberIndices.reserve(members.size());

			for (int memberIdx : members)
			{
				const auto & item = items[memberIdx];
				sumLat += item.geoCoord.latitude();
				sumLon += item.geoCoord.longitude();
				memberIndices.push_back(item.sourceIndex);
			}

			const double centroidLat = sumLat / members.size();
			const double centroidLon = sumLon / members.size();
			const QGeoCoordinate centroid(centroidLat, centroidLon);

			nodes.push_back(ClusterNode { centroid, memberIndices });
		}
	}

	return nodes;
}

void ClusterModel::OnViewportChanged(const QGeoRectangle & viewport)
{
	m_impl->viewport = viewport;

	// Rebuild clusters and update the model
	beginResetModel();
	m_impl->m_nodes = BuildClusters();
	endResetModel();
}
