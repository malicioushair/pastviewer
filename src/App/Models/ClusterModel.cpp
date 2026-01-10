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

namespace {

constexpr double CLUSTER_THRESHOLD_PIXELS = 20.0;
constexpr double CLUSTER_THRESHOLD_SQUARED = CLUSTER_THRESHOLD_PIXELS * CLUSTER_THRESHOLD_PIXELS;
constexpr int NEIGHBOR_RADIUS = 1; // Check 3x3 grid of cells

struct ClusterItem
{
	int sourceRowIndex;
	QPersistentModelIndex sourceIndex;
	QGeoCoordinate geoCoord;
	QPointF screenPos;
	int cellX;
	int cellY;
};

using CellKey = std::pair<int, int>;

struct CellKeyHash
{
	std::size_t operator()(const CellKey & key) const noexcept
	{
		// Simple hash combining for pair<int, int>
		return std::hash<int> {}(key.first) ^ (std::hash<int> {}(key.second) << 1);
	}
};

using GridMap = std::unordered_map<CellKey, std::vector<int>, CellKeyHash>;

std::vector<ClusterItem> BuildClusterItems(const QAbstractItemModel & sourceModel, const QGeoRectangle & viewport)
{
	std::vector<ClusterItem> items;
	items.reserve(sourceModel.rowCount());

	for (int i = 0; i < sourceModel.rowCount(); ++i)
	{
		const auto sourceIndex = sourceModel.index(i, 0);
		const auto coords = sourceModel.data(sourceIndex, BaseModel::Coordinate).value<QGeoCoordinate>();
		const auto zoomLevel = sourceModel.data(sourceIndex, BaseModel::ZoomLevel).toInt();
		const auto screenCoords = GeoToScreenCoords(viewport, zoomLevel, coords);

		const auto cellX = static_cast<int>(std::floor(screenCoords.x() / CLUSTER_THRESHOLD_PIXELS));
		const auto cellY = static_cast<int>(std::floor(screenCoords.y() / CLUSTER_THRESHOLD_PIXELS));

		items.push_back({ .sourceRowIndex = i,
			.sourceIndex = QPersistentModelIndex(sourceIndex),
			.geoCoord = coords,
			.screenPos = screenCoords,
			.cellX = cellX,
			.cellY = cellY });
	}

	return items;
}

GridMap BuildGridMap(const std::vector<ClusterItem> & items)
{
	GridMap gridMap;
	gridMap.reserve(items.size() / 4); // Heuristic: assume some clustering

	for (size_t i = 0; i < items.size(); ++i)
	{
		const CellKey key { items[i].cellX, items[i].cellY };
		gridMap[key].push_back(static_cast<int>(i));
	}

	return gridMap;
}

double CalculateSquaredDistance(const QPointF & a, const QPointF & b) noexcept
{
	const double dx = b.x() - a.x();
	const double dy = b.y() - a.y();
	return dx * dx + dy * dy;
}

std::vector<int> FindClusterMembers(
	const std::vector<ClusterItem> & items,
	const GridMap & gridMap,
	std::vector<bool> & visited,
	int startIndex)
{
	std::vector<int> members;
	std::queue<int> queue;

	queue.push(startIndex);
	members.push_back(startIndex);
	visited[startIndex] = true;

	while (!queue.empty())
	{
		const int currentIndex = queue.front();
		queue.pop();

		const auto & currentItem = items[currentIndex];
		const int centerCellX = currentItem.cellX;
		const int centerCellY = currentItem.cellY;

		// Check neighboring cells in 3x3 grid
		for (auto dx = -NEIGHBOR_RADIUS; dx <= NEIGHBOR_RADIUS; ++dx)
		{
			for (auto dy = -NEIGHBOR_RADIUS; dy <= NEIGHBOR_RADIUS; ++dy)
			{
				const CellKey neighborKey { centerCellX + dx, centerCellY + dy };
				const auto it = gridMap.find(neighborKey);
				if (it == gridMap.end())
					continue;

				for (const auto candidateIndex : it->second)
				{
					if (visited[candidateIndex])
						continue;

					const auto distanceSquared = CalculateSquaredDistance(currentItem.screenPos, items[candidateIndex].screenPos);

					if (distanceSquared <= CLUSTER_THRESHOLD_SQUARED)
					{
						visited[candidateIndex] = true;
						queue.push(candidateIndex);
						members.push_back(candidateIndex);
					}
				}
			}
		}
	}

	return members;
}

QGeoCoordinate CalculateCentroid(const std::vector<ClusterItem> & items, const std::vector<int> & memberIndices)
{
	auto sumLat = 0.0;
	auto sumLon = 0.0;

	for (const auto index : memberIndices)
	{
		const auto & item = items[index];
		sumLat += item.geoCoord.latitude();
		sumLon += item.geoCoord.longitude();
	}

	const auto centroidLat = sumLat / memberIndices.size();
	const auto centroidLon = sumLon / memberIndices.size();
	return { centroidLat, centroidLon };
}

Node CreateNode(const std::vector<ClusterItem> & items, const std::vector<int> & memberIndices)
{
	if (memberIndices.size() == 1)
		return IndividualNode { items[memberIndices[0]].sourceIndex };

	QVector<QPersistentModelIndex> sourceIndices;
	sourceIndices.reserve(memberIndices.size());

	for (const auto index : memberIndices)
		sourceIndices.push_back(items[index].sourceIndex);

	const auto centroid = CalculateCentroid(items, memberIndices);
	return ClusterNode { centroid, sourceIndices };
}

} // namespace

std::vector<Node> ClusterModel::BuildClusters() const
{
	const auto items = BuildClusterItems(m_impl->sourceModel, m_impl->viewport);
	if (items.empty())
		return {};

	const auto gridMap = BuildGridMap(items);
	std::vector<bool> visited(items.size(), false);
	std::vector<Node> nodes;
	nodes.reserve(items.size() / 2); // Heuristic: assume some clustering

	for (size_t i = 0; i < items.size(); ++i)
	{
		if (visited[i])
			continue;

		const auto members = FindClusterMembers(items, gridMap, visited, static_cast<int>(i));
		nodes.push_back(CreateNode(items, members));
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
