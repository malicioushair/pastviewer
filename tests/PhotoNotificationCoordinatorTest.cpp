#include <algorithm>
#include <utility>
#include <vector>

#include <QAbstractListModel>
#include <QCoreApplication>
#include <QGeoCoordinate>

#include <gtest/gtest.h>

#include "App/Controllers/ModelController/PhotoNotificationCoordinator.h"
#include "App/Models/BaseModel.h"
#include "App/Models/ScreenObjectsModel.h"
#include "App/Utils/Range.h"

namespace {

const QGeoCoordinate PHOTO_COORDINATE { 44.817812, 20.456897 };
constexpr auto ENTER_DISTANCE_METERS = 10.0;

void EnsureCoreApplication()
{
	if (QCoreApplication::instance())
		return;

	static int argc = 1;
	static char executableName[] = "PhotoNotificationCoordinatorTest";
	static char * argv[] = { executableName, nullptr };
	static QCoreApplication application(argc, argv);
}

struct MockPhoto
{
	int id;
	QGeoCoordinate coordinate;
	QString title;
	int year { 1950 };
	bool selected { false };
	bool isClustered { false };
	int zoomToDecluster { 0 };
};

class MockPhotoModel
	: public QAbstractListModel
{
public:
	int rowCount(const QModelIndex & parent = {}) const override
	{
		return parent.isValid() ? 0 : static_cast<int>(m_photos.size());
	}

	QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const override
	{
		if (!index.isValid() || index.row() < 0 || index.row() >= rowCount())
			return {};

		const auto & photo = m_photos.at(index.row());
		switch (role)
		{
			case BaseModel::Roles::Cid:
				return photo.id;
			case BaseModel::Roles::Coordinate:
				return QVariant::fromValue(photo.coordinate);
			case BaseModel::Roles::Title:
				return photo.title;
			case BaseModel::Roles::Year:
				return photo.year;
			case BaseModel::Roles::Selected:
				return photo.selected;
			case ScreenObjectsModel::Roles::IsClustered:
				return photo.isClustered;
			case ScreenObjectsModel::Roles::ZoomToDecluster:
				return photo.zoomToDecluster;
			default:
				return {};
		}
	}

	bool setData(const QModelIndex & index, const QVariant & value, int role) override
	{
		if (!index.isValid()
			|| index.row() < 0
			|| index.row() >= rowCount()
			|| role != BaseModel::Roles::Selected)
			return false;

		for (auto & photo : m_photos)
			photo.selected = false;
		m_photos.at(index.row()).selected = value.toBool();
		emit dataChanged(index, index, { role });
		return true;
	}

	QHash<int, QByteArray> roleNames() const override
	{
		return {
			{ BaseModel::Roles::Cid,                      "Cid"             },
			{ BaseModel::Roles::Coordinate,               "Coordinate"      },
			{ BaseModel::Roles::Title,                    "Title"           },
			{ BaseModel::Roles::Year,                     "Year"            },
			{ BaseModel::Roles::Selected,                 "Selected"        },
			{ ScreenObjectsModel::Roles::IsClustered,     "IsClustered"     },
			{ ScreenObjectsModel::Roles::ZoomToDecluster, "ZoomToDecluster" },
		};
	}

	void SetPhotos(std::vector<MockPhoto> photos)
	{
		beginResetModel();
		m_photos = std::move(photos);
		endResetModel();
	}

	bool IsSelected(int photoId) const
	{
		const auto photo = std::ranges::find(m_photos, photoId, &MockPhoto::id);
		return photo != m_photos.end() && photo->selected;
	}

private:
	std::vector<MockPhoto> m_photos;
};

}

TEST(PhotoNotificationCoordinatorTest, EmitsApproachUsingMockModelData)
{
	MockPhotoModel model;
	model.SetPhotos({
		{ 42, PHOTO_COORDINATE, QStringLiteral("Terazije in 1930") },
	});
	PhotoNotificationCoordinator coordinator;

	auto notificationCount = 0;
	QString notificationTitle;
	auto notificationPhotoId = 0;
	QObject::connect(&coordinator, &PhotoNotificationCoordinator::PhotoAreaApproached, [&](const QString & title, int photoId) {
		++notificationCount;
		notificationTitle = title;
		notificationPhotoId = photoId;
	});

	coordinator.EvaluatePhotoProximity(PHOTO_COORDINATE, model, ENTER_DISTANCE_METERS, true);
	coordinator.EvaluatePhotoProximity(PHOTO_COORDINATE, model, ENTER_DISTANCE_METERS, true);

	EXPECT_EQ(notificationCount, 1);
	EXPECT_EQ(notificationTitle, QStringLiteral("Terazije in 1930"));
	EXPECT_EQ(notificationPhotoId, 42);
}

TEST(PhotoNotificationCoordinatorTest, EvaluatesMockRowsLoadedAfterPositionIsKnown)
{
	MockPhotoModel model;
	PhotoNotificationCoordinator coordinator;

	auto notificationCount = 0;
	QObject::connect(&coordinator, &PhotoNotificationCoordinator::PhotoAreaApproached, [&](const QString &, int) {
		++notificationCount;
	});

	coordinator.EvaluatePhotoProximity(PHOTO_COORDINATE, model, ENTER_DISTANCE_METERS, true);
	EXPECT_EQ(notificationCount, 0);

	model.SetPhotos({
		{ 42, PHOTO_COORDINATE, QStringLiteral("Loaded later") },
	});
	coordinator.EvaluatePhotoProximity(PHOTO_COORDINATE, model, ENTER_DISTANCE_METERS, true);

	EXPECT_EQ(notificationCount, 1);
}

TEST(PhotoNotificationCoordinatorTest, DisabledEntryMustBeRearmedBeforeNotification)
{
	MockPhotoModel model;
	model.SetPhotos({
		{ 42, PHOTO_COORDINATE, QStringLiteral("Nearby photo") },
	});
	PhotoNotificationCoordinator coordinator;

	auto notificationCount = 0;
	QObject::connect(&coordinator, &PhotoNotificationCoordinator::PhotoAreaApproached, [&](const QString &, int) {
		++notificationCount;
	});

	coordinator.EvaluatePhotoProximity(PHOTO_COORDINATE, model, ENTER_DISTANCE_METERS, false);
	coordinator.EvaluatePhotoProximity(PHOTO_COORDINATE, model, ENTER_DISTANCE_METERS, true);
	EXPECT_EQ(notificationCount, 0);

	coordinator.EvaluatePhotoProximity(
		PHOTO_COORDINATE.atDistanceAndAzimuth(15.1, 0.0),
		model,
		ENTER_DISTANCE_METERS,
		true);
	coordinator.EvaluatePhotoProximity(PHOTO_COORDINATE, model, ENTER_DISTANCE_METERS, true);

	EXPECT_EQ(notificationCount, 1);
}

TEST(PhotoNotificationCoordinatorTest, SelectsPhotoAndEmitsFilteredModelMetadata)
{
	EnsureCoreApplication();
	MockPhotoModel sourceModel;
	sourceModel.SetPhotos({
		{ 42, PHOTO_COORDINATE, QStringLiteral("Visible photo"), 1950 },
	});
	ScreenObjectsModel filteredModel(&sourceModel);
	filteredModel.OnUserSelectedTimelineRangeChanged({ 1900, 2000 });
	filteredModel.UpdateZoomsToDecluster({
		{ 42, 17 }
    });
	PhotoNotificationCoordinator coordinator;

	auto selectionCount = 0;
	auto selectedRow = -1;
	QGeoCoordinate selectedCoordinate;
	auto selectedIsClustered = false;
	auto selectedZoomToDecluster = 0;
	QObject::connect(&coordinator, &PhotoNotificationCoordinator::PhotoSelected, [&](int modelIndex, const QGeoCoordinate & coordinate, bool isClustered, int zoomToDecluster) {
		++selectionCount;
		selectedRow = modelIndex;
		selectedCoordinate = coordinate;
		selectedIsClustered = isClustered;
		selectedZoomToDecluster = zoomToDecluster;
	});

	EXPECT_TRUE(coordinator.SelectPhoto(&filteredModel, 42));
	EXPECT_TRUE(sourceModel.IsSelected(42));
	EXPECT_EQ(selectionCount, 1);
	EXPECT_EQ(selectedRow, 0);
	EXPECT_EQ(selectedCoordinate, PHOTO_COORDINATE);
	EXPECT_TRUE(selectedIsClustered);
	EXPECT_EQ(selectedZoomToDecluster, 17);
}

TEST(PhotoNotificationCoordinatorTest, FilteredOutPhotoRemainsPendingUntilItBecomesVisible)
{
	EnsureCoreApplication();
	MockPhotoModel sourceModel;
	sourceModel.SetPhotos({
		{ 1, PHOTO_COORDINATE, QStringLiteral("Old photo"), 1950 },
		{ 2, PHOTO_COORDINATE, QStringLiteral("New photo"), 2010 },
	});
	ScreenObjectsModel filteredModel(&sourceModel);
	filteredModel.OnUserSelectedTimelineRangeChanged({ 1900, 2000 });
	PhotoNotificationCoordinator coordinator;

	int selectionCount = 0;
	QObject::connect(&coordinator, &PhotoNotificationCoordinator::PhotoSelected, [&](int, const QGeoCoordinate &, bool, int) {
		++selectionCount;
	});

	EXPECT_FALSE(coordinator.SelectPhoto(&filteredModel, 2));
	EXPECT_FALSE(sourceModel.IsSelected(2));
	EXPECT_EQ(selectionCount, 0);

	filteredModel.OnUserSelectedTimelineRangeChanged({ 2000, 2020 });
	EXPECT_TRUE(coordinator.RetryPendingPhotoSelection(&filteredModel));

	EXPECT_TRUE(sourceModel.IsSelected(2));
	EXPECT_EQ(selectionCount, 1);
}

TEST(PhotoNotificationCoordinatorTest, PendingSelectionSucceedsAfterMockDataLoads)
{
	EnsureCoreApplication();
	MockPhotoModel sourceModel;
	ScreenObjectsModel filteredModel(&sourceModel);
	filteredModel.OnUserSelectedTimelineRangeChanged({ 1900, 2000 });
	PhotoNotificationCoordinator coordinator;

	EXPECT_FALSE(coordinator.SelectPhoto(&filteredModel, 42));

	sourceModel.SetPhotos({
		{ 42, PHOTO_COORDINATE, QStringLiteral("Loaded after tap"), 1950 },
	});

	EXPECT_TRUE(coordinator.RetryPendingPhotoSelection(&filteredModel));
	EXPECT_TRUE(sourceModel.IsSelected(42));
	EXPECT_FALSE(coordinator.RetryPendingPhotoSelection(&filteredModel));
}
