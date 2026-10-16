#include <algorithm>
#include <memory>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>
#include <QList>
#include <QStandardPaths>
#include <QUrl>

#include <gtest/gtest.h>

#include "App/Controllers/ModelController/PositionSourceAdapter.h"
#include "App/Models/TourDraftsModel/TourDraftsModel.h"
#include "App/Tours/Tours.h"

namespace {

class FixedPositionSource : public QGeoPositionInfoSource
{
public:
    explicit FixedPositionSource(QObject * parent = nullptr)
        : QGeoPositionInfoSource(parent)
    {}

    QGeoPositionInfo lastKnownPosition(bool) const override
    {
        return m_info;
    }

    PositioningMethods supportedPositioningMethods() const override
    {
        return AllPositioningMethods;
    }

    int minimumUpdateInterval() const override
    {
        return 0;
    }

    Error error() const override
    {
        return NoError;
    }

    void startUpdates() override
    {
    }

    void stopUpdates() override
    {
    }

    void requestUpdate(int) override
    {
    }

    void SetCoordinate(const QGeoCoordinate & coordinate)
    {
        m_info = QGeoPositionInfo(coordinate, QDateTime::currentDateTimeUtc());
        emit positionUpdated(m_info);
    }

private:
    QGeoPositionInfo m_info;
};

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

void ExpectCoordinate(const QGeoCoordinate & actual, const QGeoCoordinate & expected)
{
    EXPECT_TRUE(actual == expected)
        << actual.latitude() << ", " << actual.longitude()
        << " vs " << expected.latitude() << ", " << expected.longitude();
}

void ExpectPath(const QList<QGeoCoordinate> & actual, const QList<QGeoCoordinate> & expected)
{
    EXPECT_EQ(actual.size(), expected.size());
    const auto count = std::min(actual.size(), expected.size());
    for (qsizetype index = 0; index < count; ++index)
        EXPECT_TRUE(actual.at(index) == expected.at(index))
            << index << ": " << actual.at(index).latitude() << ", " << actual.at(index).longitude()
            << " vs " << expected.at(index).latitude() << ", " << expected.at(index).longitude();
}

void CreatePlacedStop(TourDraftsModel & model, const QString & title, const QGeoCoordinate & coordinate)
{
    model.CreateTourStop(title, QStringLiteral("stop"), QUrl(), QUrl(), coordinate);
}

void CreateWalkingStop(TourDraftsModel & model, const QString & title)
{
    model.CreateTourStop(title, QStringLiteral("stop"), QUrl(), QUrl());
}

class PlacedTourStop : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        EnsureCoreApplication();
    }

    void SetUp() override
    {
        QCoreApplication::setOrganizationName(QStringLiteral("PastViewerPlacedTourStopTests"));
        QCoreApplication::setApplicationName(QStringLiteral("PastViewerPlacedTourStopTests"));
        QStandardPaths::setTestModeEnabled(true);
        QDir(Tours::GetDraftsLocation()).removeRecursively();
        ASSERT_TRUE(QDir().mkpath(Tours::GetDraftsLocation()));
        source = std::make_unique<FixedPositionSource>();
        position = std::make_unique<PositionSourceAdapter>(source.get());
        model = std::make_unique<TourDraftsModel>(*position);
    }

    void TearDown() override
    {
        model.reset();
        position.reset();
        source.reset();
        QDir(Tours::GetDraftsLocation()).removeRecursively();
    }

    std::unique_ptr<FixedPositionSource> source;
    std::unique_ptr<PositionSourceAdapter> position;
    std::unique_ptr<TourDraftsModel> model;
};

}

TEST_F(PlacedTourStop, PlacedStopAtDifferentCoordinateAppendsCoordinateAndAddsItsSegmentToDistance)
{
    model->AddTour({});
    const QGeoCoordinate first { 52.3676, 4.9041 };
    const QGeoCoordinate second { 52.3702, 4.8952 };
    const QGeoCoordinate placed { 52.3731, 4.8922 };
    model->AddPoint(first);
    model->AddPoint(second);
    ASSERT_DOUBLE_EQ(Distance(*model), first.distanceTo(second));

    CreatePlacedStop(*model, QStringLiteral("gate"), placed);

    const auto tour = CurrentTour(*model);
    ASSERT_EQ(tour.stops.size(), 1);
    EXPECT_EQ(tour.stops.at(0).title, QStringLiteral("gate"));
    ExpectCoordinate(tour.stops.at(0).coords, placed);
    ExpectPath(model->GetTourPath(), { first, second, placed });
    EXPECT_DOUBLE_EQ(Distance(*model), first.distanceTo(second) + second.distanceTo(placed));
    EXPECT_DOUBLE_EQ(tour.distance, first.distanceTo(second) + second.distanceTo(placed));
}

TEST_F(PlacedTourStop, PlacedStopAtLastPathCoordinateDoesNotAppendDuplicateOrChangeDistance)
{
    model->AddTour({});
    const QGeoCoordinate first { 48.8566, 2.3522 };
    const QGeoCoordinate last { 48.8606, 2.3376 };
    model->AddPoint(first);
    model->AddPoint(last);
    const auto distanceBefore = first.distanceTo(last);
    ASSERT_DOUBLE_EQ(Distance(*model), distanceBefore);

    const QGeoCoordinate same { last.latitude(), last.longitude() };
    CreatePlacedStop(*model, QStringLiteral("square"), same);

    const auto tour = CurrentTour(*model);
    ASSERT_EQ(tour.stops.size(), 1);
    EXPECT_EQ(tour.stops.at(0).title, QStringLiteral("square"));
    ExpectCoordinate(tour.stops.at(0).coords, last);
    ExpectPath(model->GetTourPath(), { first, last });
    EXPECT_DOUBLE_EQ(Distance(*model), distanceBefore);
    EXPECT_DOUBLE_EQ(tour.distance, distanceBefore);
}

TEST_F(PlacedTourStop, PlacedStopOnEmptyPathAppendsCoordinateAndLeavesDistanceAtZero)
{
    model->AddTour({});
    const QGeoCoordinate placed { 41.8902, 12.4922 };
    ASSERT_TRUE(model->GetTourPath().isEmpty());
    ASSERT_DOUBLE_EQ(Distance(*model), 0.0);

    CreatePlacedStop(*model, QStringLiteral("forum"), placed);

    const auto tour = CurrentTour(*model);
    ASSERT_EQ(tour.stops.size(), 1);
    EXPECT_EQ(tour.stops.at(0).title, QStringLiteral("forum"));
    ExpectCoordinate(tour.stops.at(0).coords, placed);
    ExpectPath(model->GetTourPath(), { placed });
    EXPECT_DOUBLE_EQ(Distance(*model), 0.0);
    EXPECT_DOUBLE_EQ(tour.distance, 0.0);

    const QGeoCoordinate next { 41.9028, 12.4964 };
    model->AddPoint(next);
    ExpectPath(model->GetTourPath(), { placed, next });
    EXPECT_DOUBLE_EQ(Distance(*model), placed.distanceTo(next));
}

TEST_F(PlacedTourStop, StopWithoutPlacementCoordinateDoesNotAppendPathPoint)
{
    const QGeoCoordinate here { 40.7484, -73.9857 };
    source->SetCoordinate(here);
    ASSERT_TRUE(position->Coordinate() == here);

    model->AddTour({});
    CreateWalkingStop(*model, QStringLiteral("walk"));

    auto tour = CurrentTour(*model);
    ASSERT_EQ(tour.stops.size(), 1);
    ExpectCoordinate(tour.stops.at(0).coords, here);
    EXPECT_TRUE(model->GetTourPath().isEmpty());
    EXPECT_DOUBLE_EQ(Distance(*model), 0.0);

    const QGeoCoordinate first { 52.3676, 4.9041 };
    const QGeoCoordinate second { 52.3702, 4.8952 };
    model->AddPoint(first);
    model->AddPoint(second);
    const auto distanceBefore = first.distanceTo(second);
    ASSERT_DOUBLE_EQ(Distance(*model), distanceBefore);

    CreateWalkingStop(*model, QStringLiteral("bridge"));

    tour = CurrentTour(*model);
    ASSERT_EQ(tour.stops.size(), 2);
    EXPECT_EQ(tour.stops.at(1).title, QStringLiteral("bridge"));
    ExpectCoordinate(tour.stops.at(1).coords, here);
    ExpectPath(model->GetTourPath(), { first, second });
    EXPECT_DOUBLE_EQ(Distance(*model), distanceBefore);
    EXPECT_DOUBLE_EQ(tour.distance, distanceBefore);
}
