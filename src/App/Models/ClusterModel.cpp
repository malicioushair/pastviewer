#include "ClusterModel.h"

#include "App/Models/BaseModel.h"

#include <functional>
#include <numbers>
#include <queue>
#include <unordered_map>
#include <variant>
#include <vector>

#include <QPointF>

namespace {

constexpr auto CLUSTER_THRESHOLD_PIXELS = 20.0;
constexpr auto CLUSTER_THRESHOLD_SQUARED = CLUSTER_THRESHOLD_PIXELS * CLUSTER_THRESHOLD_PIXELS;
constexpr auto NEIGHBOR_RADIUS = 1; // Check 3x3 grid of cells

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

const auto getHash = [](const CellKey & key) {
	return std::hash<int> {}(key.first) ^ (std::hash<int> {}(key.second) << 1);
};

using GridMap = std::unordered_map<CellKey, std::vector<int>, decltype(getHash)>;

double ClampLat(double lat) noexcept
{
	// Maximum latitude for Web Mercator projection (EPSG:3857)
	// Formula: arctan(sinh(π)) × 180/π ≈ 85.0511287798°
	// This is the latitude where Mercator projection becomes infinite
	// Used by Google Maps, OpenStreetMap, and most web mapping services
	constexpr auto WEB_MERCATOR_MAX_LATITUDE = 85.0511287798;

	if (lat > WEB_MERCATOR_MAX_LATITUDE)
		return WEB_MERCATOR_MAX_LATITUDE;
	if (lat < -WEB_MERCATOR_MAX_LATITUDE)
		return -WEB_MERCATOR_MAX_LATITUDE;
	return lat;
}

double WrapLon(double lon) noexcept
{
	auto x = std::fmod(lon + 180.0, 360.0);
	if (x < 0)
		x += 360.0;
	return x - 180.0; // [-180, 180)
}

double WorldSizeForZoom(const int zoom) noexcept
{
	return std::ldexp(256.0, zoom);
}

QPointF GeoToWorldPx(const QGeoCoordinate & c, const int z)
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

std::vector<ClusterItem> BuildClusterItems(const QAbstractItemModel & sourceModel, const QGeoRectangle & viewport)
{
	std::vector<ClusterItem> items;
	items.reserve(sourceModel.rowCount());
	std::unordered_set<int> seenCids;

	for (int i = 0; i < sourceModel.rowCount(); ++i)
	{
		const auto sourceIndex = sourceModel.index(i, 0);
		const auto cid = sourceModel.data(sourceIndex, BaseModel::Cid).toInt();
		if (seenCids.contains(cid))
			continue;
		seenCids.insert(cid);

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
	gridMap.reserve(items.size());

	for (size_t i = 0; i < items.size(); ++i)
	{
		const CellKey key { items[i].cellX, items[i].cellY };
		gridMap[key].push_back(static_cast<int>(i));
	}

	return gridMap;
}

double CalculateSquaredDistance(const QPointF & a, const QPointF & b) noexcept
{
	const auto dx = b.x() - a.x();
	const auto dy = b.y() - a.y();
	return dx * dx + dy * dy;
}

void VisitNeighboringItems(
	const std::vector<ClusterItem> & items,
	const GridMap & gridMap,
	const int itemIndex,
	std::function<void(int, double)> && callback)
{
	const auto & item = items.at(itemIndex);

	// Check 3x3 grid of cells
	for (auto dxCell = -NEIGHBOR_RADIUS; dxCell <= NEIGHBOR_RADIUS; ++dxCell)
	{
		for (auto dyCell = -NEIGHBOR_RADIUS; dyCell <= NEIGHBOR_RADIUS; ++dyCell)
		{
			const CellKey key { item.cellX + dxCell, item.cellY + dyCell };
			const auto gridIt = gridMap.find(key);
			if (gridIt == gridMap.end())
				continue;

			// Check all items in this cell
			for (const auto otherIndex : gridIt->second)
			{
				if (const auto isSelf = otherIndex == itemIndex; isSelf)
					continue;

				const auto squaredDistance = CalculateSquaredDistance(item.screenPos, items.at(otherIndex).screenPos);
				callback(otherIndex, squaredDistance);
			}
		}
	}
}

std::vector<int> FindClusterMembers(
	const std::vector<ClusterItem> & items,
	const GridMap & gridMap,
	std::vector<bool> & visited,
	const int startIndex)
{
	std::vector<int> members;
	std::queue<int> queue;

	queue.push(startIndex);
	members.push_back(startIndex);
	visited[startIndex] = true;

	while (!queue.empty())
	{
		const auto currentIndex = queue.front();
		queue.pop();

		VisitNeighboringItems(items, gridMap, currentIndex, [&](int candidateIndex, double distanceSquared) {
			if (visited.at(candidateIndex))
				return;

			if (distanceSquared <= CLUSTER_THRESHOLD_SQUARED)
			{
				visited.at(candidateIndex) = true;
				queue.push(candidateIndex);
				members.push_back(candidateIndex);
			}
		});
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

	assert(!memberIndices.empty());
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

double FindNearestNeighborDistancePx(const std::vector<ClusterItem> & items, const GridMap & gridMap, const int itemIndex)
{
	auto bestDistanceSquared = std::numeric_limits<double>::infinity();

	VisitNeighboringItems(items, gridMap, itemIndex, [&](int, double squaredDistance) {
		if (squaredDistance < bestDistanceSquared)
			bestDistanceSquared = squaredDistance;
	});

	return std::isfinite(bestDistanceSquared)
			 ? std::sqrt(bestDistanceSquared)
			 : std::numeric_limits<double>::infinity();
}

int ZoomToDeclusterFromNearestPx(const double dPx, const int currentZoom)
{
	static constexpr auto maxZoom = 20;

	if (dPx <= 0.0)
		return std::min(maxZoom, currentZoom + 1);

	if (const auto alreadyDeclustered = dPx > CLUSTER_THRESHOLD_PIXELS; alreadyDeclustered)
		return currentZoom;

	// How many zooms(doublings) until dPx becomes > 20?
	// After k zoom steps: dPx * 2^k > 20
	// So: k > log2(20 / dPx)
	const auto zooms = static_cast<int>(std::ceil(std::log2(CLUSTER_THRESHOLD_PIXELS / dPx)));

	return std::min(maxZoom, currentZoom + std::max(1, zooms));
}

}

struct ClusterModel::Impl
{
	QAbstractItemModel * sourceModel;
	std::vector<Node> nodes;
	QGeoRectangle viewport;
};

ClusterModel::ClusterModel(QAbstractItemModel * sourceModel, QObject * parent)
	: QAbstractListModel(parent)
	, m_impl(std::make_unique<Impl>(sourceModel))

{
	connect(m_impl->sourceModel, &QAbstractItemModel::modelReset, this, &ClusterModel::modelReset);
	connect(m_impl->sourceModel, &QAbstractItemModel::rowsInserted, this, &ClusterModel::rowsInserted);
	connect(m_impl->sourceModel, &QAbstractItemModel::rowsRemoved, this, &ClusterModel::rowsRemoved);
	connect(m_impl->sourceModel, &QAbstractItemModel::dataChanged, this, &ClusterModel::dataChanged);

	connect(this, &ClusterModel::rowsInserted, this, [this] { emit CountChanged(); });
	connect(this, &ClusterModel::rowsRemoved, this, [this] { emit CountChanged(); });
	connect(this, &ClusterModel::modelReset, this, [this] { emit CountChanged(); });

	beginResetModel();
	m_impl->nodes = BuildClusters();
	endResetModel();
}

ClusterModel::~ClusterModel() = default;

int ClusterModel::rowCount(const QModelIndex & parent) const
{
	return m_impl->nodes.size();
}

QVariant ClusterModel::data(const QModelIndex & index, int role) const
{
	if (!index.isValid() || index.row() < 0 || index.row() >= m_impl->nodes.size())
		return {};

	const auto & node = m_impl->nodes[index.row()];
	switch (role)
	{
		case IsCluster:
			return std::holds_alternative<ClusterNode>(node);
		case ClusterCount:
		{
			if (!std::holds_alternative<ClusterNode>(node))
				return 1;

			const auto & clusterNode = std::get<ClusterNode>(node);
			return clusterNode.indicesIntoSourceModel.size();
		}
		case CidsInCluster:
		{
			if (!std::holds_alternative<ClusterNode>(node))
				return {};

			const auto & clusterNode = std::get<ClusterNode>(node);
			QVariantList cids;
			cids.reserve(clusterNode.indicesIntoSourceModel.size());
			for (const auto index : clusterNode.indicesIntoSourceModel)
				cids.emplace_back(m_impl->sourceModel->data(index, BaseModel::Roles::Cid).toInt());

			return cids;
		}
	}

	if (std::holds_alternative<IndividualNode>(node))
	{
		const auto & individualNode = std::get<IndividualNode>(node);
		assert(individualNode.indexIntoSourceModel.isValid());
		return m_impl->sourceModel->data(individualNode.indexIntoSourceModel, role);
	}
	else if (std::holds_alternative<ClusterNode>(node))
	{
		const auto & clusterNode = std::get<ClusterNode>(node);

		// For clusters, return centroid coordinate and aggregate data from first member
		if (role == BaseModel::Coordinate)
			return QVariant::fromValue(clusterNode.centroid);

		// For other roles, use data from the first member
		if (!clusterNode.indicesIntoSourceModel.isEmpty() && clusterNode.indicesIntoSourceModel.front().isValid())
			return m_impl->sourceModel->data(clusterNode.indicesIntoSourceModel.front(), role);
	}

	return {};
}

QHash<int, QByteArray> ClusterModel::roleNames() const
{
	auto roles = m_impl->sourceModel->roleNames();
#define ROLENAME(NAME) roles[NAME] = #NAME
	ROLENAME(ClusterCount);
	ROLENAME(CidsInCluster);
	ROLENAME(IsCluster);
#undef ROLENAME
	return roles;
}

std::vector<Node> ClusterModel::BuildClusters() const
{
	const auto items = BuildClusterItems(*m_impl->sourceModel, m_impl->viewport);
	if (items.empty())
		return {};

	const auto gridMap = BuildGridMap(items);
	std::vector<bool> visited(items.size(), false);
	std::vector<Node> nodes;
	nodes.reserve(items.size());

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

	const auto items = BuildClusterItems(*m_impl->sourceModel, viewport);
	const auto gridMap = BuildGridMap(items);

	const auto currentZoom = m_impl->sourceModel->data({}, BaseModel::ZoomLevel).toInt();
	QHash<int, int> cidToZoom;
	for (size_t i = 0; i < items.size(); ++i)
	{
		const auto sourceIndex = items[i].sourceIndex;
		const auto cid = m_impl->sourceModel->data(sourceIndex, BaseModel::Cid).toInt();
		const auto dPx = FindNearestNeighborDistancePx(items, gridMap, static_cast<int>(i));
		const auto zoomToDecluster = ZoomToDeclusterFromNearestPx(dPx, currentZoom);
		cidToZoom[cid] = zoomToDecluster;
	}

	emit ZoomsToDecluster(cidToZoom);

	beginResetModel();
	m_impl->nodes = BuildClusters();
	endResetModel();
}
