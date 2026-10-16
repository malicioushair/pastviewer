#include <algorithm>
#include <memory>

#include <QCoreApplication>
#include <QDir>
#include <QGeoCoordinate>
#include <QList>
#include <QStandardPaths>

#include <gtest/gtest.h>

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Models/TourDraftsModel/TourDraftsModel.h"
#include "App/Tours/Tours.h"

namespace {

void EnsureCoreApplication()
{
	if (QCoreApplication::instance())
		return;

	// Leak the application. TourDraftsModel's QNetworkAccessManager leaves
	// process-lifetime QObjects that crash if it is destroyed first.
	static int argc = 1;
	static char executableName[] = "PastViewerTests";
	static char * argv[] = { executableName, nullptr };
	new QCoreApplication(argc, argv);
}

template <typename Model>
void AddPoint(Model & model, const QGeoCoordinate & coordinate)
{
	if constexpr (requires { model.AddPoint(coordinate); })
		model.AddPoint(coordinate);
	else {
		static_cast<void>(model);
		static_cast<void>(coordinate);
		ADD_FAILURE() << "TourDraftsModel::AddPoint is not implemented";
	}
}

template <typename Model>
void RemoveLastPoint(Model & model)
{
	if constexpr (requires { model.RemoveLastPoint(); })
		model.RemoveLastPoint();
	else {
		static_cast<void>(model);
		ADD_FAILURE() << "TourDraftsModel::RemoveLastPoint is not implemented";
	}
}

double Distance(const TourDraftsModel & model)
{
	const auto drafts = model.GetTourDrafts();
	EXPECT_FALSE(drafts.empty());
	if (drafts.empty())
		return 0.0;
	return drafts.back().distance;
}

Tours::Tour CurrentTour(const TourDraftsModel & model)
{
	const auto drafts = model.GetTourDrafts();
	EXPECT_EQ(drafts.size(), std::size_t { 1 });
	if (drafts.empty())
		return {};
	return drafts.back();
}

void ExpectPath(const QList<QGeoCoordinate> & actual, const QList<QGeoCoordinate> & expected)
{
	EXPECT_EQ(actual.size(), expected.size());
	const auto count = std::min(actual.size(), expected.size());
	for (qsizetype index = 0; index < count; ++index) {
		EXPECT_TRUE(actual.at(index) == expected.at(index))
			<< index << ": " << actual.at(index).latitude() << ", " << actual.at(index).longitude()
			<< " vs " << expected.at(index).latitude() << ", " << expected.at(index).longitude();
	}
}

Tours::TourStop Stop(int64_t id, const QString & title, const QGeoCoordinate & coordinate)
{
	return {
		.id = id,
		.title = title,
		.coords = coordinate,
	};
}

bool HasStop(const QList<Tours::TourStop> & stops, const QString & title, const QGeoCoordinate & coordinate)
{
	for (const auto & stop : stops) {
		if (stop.title == title)
			return stop.coords == coordinate;
	}
	return false;
}

bool HasStopTitle(const QList<Tours::TourStop> & stops, const QString & title)
{
	for (const auto & stop : stops) {
		if (stop.title == title)
			return true;
	}
	return false;
}

class PlacedTourPath : public ::testing::Test
{
protected:
	static void SetUpTestSuite()
	{
		EnsureCoreApplication();
	}

	void SetUp() override
	{
		QCoreApplication::setOrganizationName(QStringLiteral("PastViewerPlacedTourPathTests"));
		QCoreApplication::setApplicationName(QStringLiteral("PastViewerPlacedTourPathTests"));
		QStandardPaths::setTestModeEnabled(true);
		QDir(Tours::GetDraftsLocation()).removeRecursively();
		ASSERT_TRUE(QDir().mkpath(Tours::GetDraftsLocation()));
		position = std::make_unique<PositionSourceAdapter>(nullptr);
		model = std::make_unique<TourDraftsModel>(*position);
	}

	void TearDown() override
	{
		model.reset();
		position.reset();
		QDir(Tours::GetDraftsLocation()).removeRecursively();
	}

	std::unique_ptr<PositionSourceAdapter> position;
	std::unique_ptr<TourDraftsModel> model;
};

}

TEST_F(PlacedTourPath, EmptyDraftHasEmptyPathAndZeroDistance)
{
	model->AddTour({});

	EXPECT_TRUE(model->GetTourPath().isEmpty());
	EXPECT_DOUBLE_EQ(Distance(*model), 0.0);
}

TEST_F(PlacedTourPath, NewTourRefreshesExistingObserversOfStopsPathAndDistance)
{
	const QGeoCoordinate first { 52.3676, 4.9041 };
	const QGeoCoordinate second { 52.3702, 4.8952 };
	model->AddTour({
		.id = 1,
		.title = QStringLiteral("Previous draft"),
		.distance = first.distanceTo(second),
		.tourPath = { first, second },
		.stops = { Stop(1, QStringLiteral("square"), second) },
	});

	// Like persistent QML bindings, refresh cached values only on notification.
	auto visibleStops = model->GetCurrentStopCoordinates();
	auto visiblePath = model->GetTourPath();
	auto visibleDistance = Distance(*model);
	QObject observer;
	QObject::connect(model.get(), &TourDraftsModel::StopsUpdated, &observer, [&] {
		visibleStops = model->GetCurrentStopCoordinates();
	});
	QObject::connect(model.get(), &TourDraftsModel::CurrentTourPathChanged, &observer, [&] {
		visiblePath = model->GetTourPath();
	});
	QObject::connect(model.get(), &TourDraftsModel::CurrentDistanceChanged, &observer, [&] {
		visibleDistance = Distance(*model);
	});

	model->CreateNewTour(QStringLiteral("New draft"), QStringLiteral("Description"), QUrl());

	const auto drafts = model->GetTourDrafts();
	ASSERT_EQ(drafts.size(), std::size_t { 2 });
	EXPECT_TRUE(visibleStops.isEmpty());
	EXPECT_TRUE(visiblePath.isEmpty());
	EXPECT_DOUBLE_EQ(visibleDistance, 0.0);
	EXPECT_EQ(drafts.front().stops.size(), 1);
	ExpectPath(drafts.front().tourPath, { first, second });
}

TEST_F(PlacedTourPath, AddedCoordinatesAppendInOrderAndDistanceIsSumOfSegments)
{
	model->AddTour({});
	const QGeoCoordinate first { 52.3676, 4.9041 };
	const QGeoCoordinate second { 52.3702, 4.8952 };
	const QGeoCoordinate third { 52.3731, 4.8922 };

	AddPoint(*model, first);
	ExpectPath(model->GetTourPath(), { first });
	EXPECT_DOUBLE_EQ(Distance(*model), 0.0);

	AddPoint(*model, second);
	ExpectPath(model->GetTourPath(), { first, second });
	EXPECT_DOUBLE_EQ(Distance(*model), first.distanceTo(second));

	AddPoint(*model, third);
	ExpectPath(model->GetTourPath(), { first, second, third });
	EXPECT_DOUBLE_EQ(Distance(*model), first.distanceTo(second) + second.distanceTo(third));
}

TEST_F(PlacedTourPath, PointPlacedOnPreviousPointIsStillAppended)
{
	model->AddTour({});
	const QGeoCoordinate first { 48.8566, 2.3522 };
	const QGeoCoordinate repeated { 48.8606, 2.3376 };

	AddPoint(*model, first);
	AddPoint(*model, repeated);
	AddPoint(*model, QGeoCoordinate(repeated.latitude(), repeated.longitude()));

	ExpectPath(model->GetTourPath(), { first, repeated, repeated });
	EXPECT_DOUBLE_EQ(Distance(*model), first.distanceTo(repeated) + repeated.distanceTo(repeated));
}

TEST_F(PlacedTourPath, RemoveLastPointDropsCoordinateSegmentAndStopsAtThatCoordinate)
{
	const QGeoCoordinate first { 52.3676, 4.9041 };
	const QGeoCoordinate second { 52.3702, 4.8952 };
	const QGeoCoordinate third { 52.3731, 4.8922 };
	const auto fullDistance = first.distanceTo(second) + second.distanceTo(third);

	model->AddTour({
		.id = 1,
		.title = QStringLiteral("Placed draft"),
		.distance = fullDistance,
		.tourPath = { first, second, third },
		.stops = {
			Stop(1, QStringLiteral("gate"), first),
			Stop(2, QStringLiteral("square"), third),
			Stop(3, QStringLiteral("bridge"), second),
			Stop(4, QStringLiteral("square-again"), QGeoCoordinate(third.latitude(), third.longitude())),
		},
	});

	ASSERT_DOUBLE_EQ(Distance(*model), fullDistance);
	ASSERT_EQ(model->GetTourPath().size(), 3);
	ASSERT_EQ(CurrentTour(*model).stops.size(), 4);

	RemoveLastPoint(*model);

	const auto tour = CurrentTour(*model);
	const auto path = model->GetTourPath();
	EXPECT_EQ(path.size(), 2);
	EXPECT_DOUBLE_EQ(tour.distance, first.distanceTo(second));
	EXPECT_DOUBLE_EQ(Distance(*model), first.distanceTo(second));
	if (!path.isEmpty()) {
		EXPECT_TRUE(path.constLast() == second)
			<< path.constLast().latitude() << ", " << path.constLast().longitude();
	}
	if (path.size() >= 2) {
		EXPECT_TRUE(path.at(0) == first);
		EXPECT_TRUE(path.at(1) == second);
	}

	EXPECT_EQ(tour.stops.size(), 2);
	EXPECT_TRUE(HasStop(tour.stops, QStringLiteral("gate"), first));
	EXPECT_TRUE(HasStop(tour.stops, QStringLiteral("bridge"), second));
	EXPECT_FALSE(HasStopTitle(tour.stops, QStringLiteral("square")));
	EXPECT_FALSE(HasStopTitle(tour.stops, QStringLiteral("square-again")));
}

TEST_F(PlacedTourPath, RemoveLastPointOnEmptyPathLeavesPathEmptyAndDistanceZero)
{
	model->AddTour({});

	RemoveLastPoint(*model);

	EXPECT_TRUE(model->GetTourPath().isEmpty());
	EXPECT_DOUBLE_EQ(Distance(*model), 0.0);
	EXPECT_TRUE(CurrentTour(*model).stops.isEmpty());
}
