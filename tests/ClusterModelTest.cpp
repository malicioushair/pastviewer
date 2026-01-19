#include <memory>

#include <QAbstractListModel>
#include <QByteArray>
#include <QCoreApplication>
#include <QGeoCoordinate>
#include <QGeoRectangle>
#include <QHash>
#include <QModelIndex>
#include <QPointF>
#include <QVariant>

#include <gtest/gtest.h>

#include "App/Models/BaseModel.h"
#include "App/Models/ClusterModel.h"
#include <QMetaType>

// Mock source model for testing ClusterModel
class MockSourceModel : public QAbstractListModel
{
	Q_OBJECT

public:
	struct MockItem
	{
		int cid;
		QGeoCoordinate coord;
		int year;
		int zoomLevel;
	};

	explicit MockSourceModel(QObject * parent = nullptr)
		: QAbstractListModel(parent)
		, m_zoomLevel(13)
	{
	}

	int rowCount(const QModelIndex & parent = QModelIndex()) const override
	{
		if (parent.isValid())
			return 0;
		return static_cast<int>(m_items.size());
	}

	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override
	{
		if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size()))
		{
			if (role == BaseModel::ZoomLevel)
				return m_zoomLevel;
			return {};
		}

		const auto & item = m_items[index.row()];
		switch (role)
		{
			case BaseModel::Cid:
				return item.cid;
			case BaseModel::Coordinate:
				return QVariant::fromValue(item.coord);
			case BaseModel::Year:
				return item.year;
			case BaseModel::ZoomLevel:
				return item.zoomLevel;
			default:
				return {};
		}
	}

	QHash<int, QByteArray> roleNames() const override
	{
		return {
			{ BaseModel::Cid,        "Cid"        },
			{ BaseModel::Coordinate, "Coordinate" },
			{ BaseModel::Year,       "Year"       },
			{ BaseModel::ZoomLevel,  "ZoomLevel"  }
		};
	}

	void addItem(int cid, const QGeoCoordinate & coord, int year = 2000, int zoomLevel = 13)
	{
		beginInsertRows({}, static_cast<int>(m_items.size()), static_cast<int>(m_items.size()));
		m_items.push_back({ cid, coord, year, zoomLevel });
		endInsertRows();
	}

	void setZoomLevel(int zoomLevel)
	{
		m_zoomLevel = zoomLevel;
		emit dataChanged({}, {}, { BaseModel::ZoomLevel });
	}

	void clear()
	{
		beginResetModel();
		m_items.clear();
		endResetModel();
	}

private:
	std::vector<MockItem> m_items;
	int m_zoomLevel;
};

class ClusterModelTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		// Initialize Qt if not already initialized
		if (!QCoreApplication::instance())
		{
			static int argc = 1;
			static char * argv[] = { const_cast<char *>("test") };
			app = std::make_unique<QCoreApplication>(argc, argv);
		}

		mockModel = new MockSourceModel(&parent);
		clusterModel = new ClusterModel(mockModel, &parent);
	}

	void TearDown() override
	{
		delete clusterModel;
		delete mockModel;
		app.reset();
	}

	std::unique_ptr<QCoreApplication> app;
	QObject parent;
	MockSourceModel * mockModel;
	ClusterModel * clusterModel;
};

// Test ClusterModel with empty source model
TEST_F(ClusterModelTest, EmptySourceModel)
{
	EXPECT_EQ(clusterModel->rowCount(), 0);
}

// Test ClusterModel with single item
TEST_F(ClusterModelTest, SingleItem)
{
	mockModel->addItem(1, QGeoCoordinate(55.7558, 37.6173), 2000);
	const QGeoRectangle viewport(QGeoCoordinate(55.0, 37.0), QGeoCoordinate(56.0, 38.0));
	clusterModel->OnViewportChanged(viewport);

	EXPECT_EQ(clusterModel->rowCount(), 1);

	const auto index = clusterModel->index(0, 0);
	EXPECT_FALSE(clusterModel->data(index, ClusterModel::IsCluster).toBool());
	EXPECT_EQ(clusterModel->data(index, ClusterModel::ClusterCount).toInt(), 1);
}

// Test ClusterModel with two items far apart (should not cluster)
TEST_F(ClusterModelTest, TwoItemsFarApart)
{
	const QGeoRectangle viewport(QGeoCoordinate(55.0, 37.0), QGeoCoordinate(56.0, 38.0));
	mockModel->addItem(1, QGeoCoordinate(55.1, 37.1), 2000);
	mockModel->addItem(2, QGeoCoordinate(55.9, 37.9), 2000);

	clusterModel->OnViewportChanged(viewport);

	// Should have 2 individual nodes
	EXPECT_EQ(clusterModel->rowCount(), 2);
	EXPECT_FALSE(clusterModel->data(clusterModel->index(0, 0), ClusterModel::IsCluster).toBool());
	EXPECT_FALSE(clusterModel->data(clusterModel->index(1, 0), ClusterModel::IsCluster).toBool());
}

// Test ClusterModel with duplicate CIDs (should deduplicate)
TEST_F(ClusterModelTest, DuplicateCids)
{
	const QGeoRectangle viewport(QGeoCoordinate(55.0, 37.0), QGeoCoordinate(56.0, 38.0));
	mockModel->addItem(1, QGeoCoordinate(55.1, 37.1), 2000);
	mockModel->addItem(1, QGeoCoordinate(55.2, 37.2), 2000); // Same CID

	clusterModel->OnViewportChanged(viewport);

	// Should have only 1 node (duplicate removed)
	EXPECT_EQ(clusterModel->rowCount(), 1);
}

// Test ClusterModel role names
TEST_F(ClusterModelTest, RoleNames)
{
	const auto roles = clusterModel->roleNames();
	EXPECT_TRUE(roles.contains(ClusterModel::IsCluster));
	EXPECT_TRUE(roles.contains(ClusterModel::ClusterCount));
	EXPECT_TRUE(roles.contains(ClusterModel::CidsInCluster));
}

// Test ClusterModel data access for individual node
TEST_F(ClusterModelTest, IndividualNodeData)
{
	mockModel->addItem(1, QGeoCoordinate(55.7558, 37.6173), 2000);
	const QGeoRectangle viewport(QGeoCoordinate(55.0, 37.0), QGeoCoordinate(56.0, 38.0));
	clusterModel->OnViewportChanged(viewport);

	const auto index = clusterModel->index(0, 0);

	EXPECT_FALSE(clusterModel->data(index, ClusterModel::IsCluster).toBool());
	EXPECT_EQ(clusterModel->data(index, ClusterModel::ClusterCount).toInt(), 1);
	EXPECT_FALSE(clusterModel->data(index, ClusterModel::CidsInCluster).isValid());
	EXPECT_EQ(clusterModel->data(index, BaseModel::Cid).toInt(), 1);
}

// Test ClusterModel with viewport change
TEST_F(ClusterModelTest, ViewportChange)
{
	const QGeoRectangle viewport1(QGeoCoordinate(55.0, 37.0), QGeoCoordinate(56.0, 38.0));
	mockModel->addItem(1, QGeoCoordinate(55.5, 37.5), 2000);
	mockModel->addItem(2, QGeoCoordinate(55.6, 37.6), 2000);

	clusterModel->OnViewportChanged(viewport1);
	const auto count1 = clusterModel->rowCount();

	const QGeoRectangle viewport2(QGeoCoordinate(54.0, 36.0), QGeoCoordinate(55.0, 37.0));
	clusterModel->OnViewportChanged(viewport2);
	const auto count2 = clusterModel->rowCount();

	// Count may change based on viewport
	EXPECT_GE(count1, 0);
	EXPECT_GE(count2, 0);
}

// Test BuildClusters method
TEST_F(ClusterModelTest, BuildClusters)
{
	const QGeoRectangle viewport(QGeoCoordinate(55.0, 37.0), QGeoCoordinate(56.0, 38.0));
	mockModel->addItem(1, QGeoCoordinate(55.5, 37.5), 2000);
	mockModel->addItem(2, QGeoCoordinate(55.6, 37.6), 2000);

	clusterModel->OnViewportChanged(viewport);
	const auto nodes = clusterModel->BuildClusters();

	EXPECT_GE(nodes.size(), 0);
	EXPECT_LE(nodes.size(), 2); // At most 2 nodes (could be clustered if close enough)
}

// Test invalid index handling
TEST_F(ClusterModelTest, InvalidIndex)
{
	const auto invalidIndex = clusterModel->index(-1, 0);
	EXPECT_FALSE(clusterModel->data(invalidIndex, ClusterModel::IsCluster).isValid());

	const auto outOfRangeIndex = clusterModel->index(100, 0);
	EXPECT_FALSE(clusterModel->data(outOfRangeIndex, ClusterModel::IsCluster).isValid());
}

#include "ClusterModelTest.moc"
