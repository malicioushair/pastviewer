#include <memory>
#include <optional>
#include <type_traits>

#include <QCoreApplication>
#include <QEventLoop>
#include <QString>
#include <QTimer>

#include <gtest/gtest.h>

#include "App/Tours/DisplayTours.h"

#if __has_include("App/Controllers/PurchaseController/PurchaseController.h")
    #include "App/Controllers/PurchaseController/PurchaseController.h"
    #define PASTVIEWER_HAS_PURCHASE_CONTROLLER 1
#else
    #define PASTVIEWER_HAS_PURCHASE_CONTROLLER 0
#endif

namespace {

// Long enough for a queued emission, short enough that a store query which never
// answers fails the test instead of hanging it. These tests do not call StoreKit.
constexpr int kOwnershipSignalTimeoutMs = 1000;

void EnsureCoreApplication()
{
    if (QCoreApplication::instance())
        return;

    // Leak the application. Controllers can leave process-lifetime QObjects
    // that crash if the application is destroyed first.
    static int argc = 1;
    static char executableName[] = "PastViewerTests";
    static char * argv[] = { executableName, nullptr };
    new QCoreApplication(argc, argv);
}

struct OwnershipEmission
{
    QString tourId;
    bool owned = false;
};

DisplayTour MakeTour(const QString & id, bool paid)
{
    DisplayTour tour;
    tour.id = id;
    tour.purchaseInfo.paid = paid;
    tour.purchaseInfo.appleId = QStringLiteral("com.pastviewer.tours.") + id;
    tour.purchaseInfo.googleId = QStringLiteral("com.pastviewer.tours.") + id;
    return tour;
}

#if PASTVIEWER_HAS_PURCHASE_CONTROLLER

template <typename Controller>
concept HasTourOwnershipApi = requires(Controller & controller, const DisplayTour & tour) {
    controller.CheckTourOwned(tour);
    &Controller::tourOwned;
};

template <typename Controller>
std::optional<OwnershipEmission> WaitForTourOwned(Controller & controller, const DisplayTour & tour)
{
    if constexpr (!HasTourOwnershipApi<Controller>)
    {
        ADD_FAILURE() << "PurchaseController must expose CheckTourOwned and tourOwned";
        return std::nullopt;
    }
    else
    {
        std::optional<OwnershipEmission> emission;
        QEventLoop loop;
        const auto connection = QObject::connect(
            &controller,
            &Controller::tourOwned,
            &loop,
            [&](const QString & tourId, bool owned) {
                emission = OwnershipEmission { tourId, owned };
                loop.quit();
            });

        controller.CheckTourOwned(tour);
        if (!emission)
        {
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeout.start(kOwnershipSignalTimeoutMs);
            loop.exec();
        }

        QObject::disconnect(connection);
        return emission;
    }
}

template <typename Controller>
std::unique_ptr<Controller> MakePurchaseController()
{
    if constexpr (std::is_constructible_v<Controller>)
        return std::make_unique<Controller>();
    else if constexpr (std::is_constructible_v<Controller, QObject *>)
        return std::make_unique<Controller>(nullptr);
    else
    {
        ADD_FAILURE() << "PurchaseController must be constructible for unit tests";
        return nullptr;
    }
}

#endif

class TourPurchaseOwnership : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        EnsureCoreApplication();
    }

#if PASTVIEWER_HAS_PURCHASE_CONTROLLER
    void SetUp() override
    {
        controller = MakePurchaseController<PurchaseController>();
        ASSERT_NE(controller, nullptr);
    }

    void TearDown() override
    {
        controller.reset();
    }

    std::unique_ptr<PurchaseController> controller;
#endif
};

} // namespace

TEST_F(TourPurchaseOwnership, UnpaidTourEmitsOwned)
{
#if !PASTVIEWER_HAS_PURCHASE_CONTROLLER
    GTEST_FAIL() << "PurchaseController is not implemented; ownership must move off TourController";
#else
    ASSERT_NE(controller, nullptr);

    const auto tour = MakeTour(QStringLiteral("unpaid"), false);
    const auto emission = WaitForTourOwned(*controller, tour);

    ASSERT_TRUE(emission.has_value())
        << "CheckTourOwned did not emit tourOwned within " << kOwnershipSignalTimeoutMs << "ms";
    EXPECT_EQ(emission->tourId, tour.id);
    EXPECT_TRUE(emission->owned) << "purchase.paid false must emit tourOwned with owned true";
#endif
}

TEST_F(TourPurchaseOwnership, PaidTourOnDesktopEmitsNotOwned)
{
#if !PASTVIEWER_HAS_PURCHASE_CONTROLLER
    GTEST_FAIL() << "PurchaseController is not implemented; ownership must move off TourController";
#else
    ASSERT_NE(controller, nullptr);

    const auto tour = MakeTour(QStringLiteral("paid"), true);
    const auto emission = WaitForTourOwned(*controller, tour);

    ASSERT_TRUE(emission.has_value())
        << "CheckTourOwned did not emit tourOwned within " << kOwnershipSignalTimeoutMs << "ms";
    EXPECT_EQ(emission->tourId, tour.id);
    EXPECT_FALSE(emission->owned) << "a paid tour on desktop must emit tourOwned with owned false";
#endif
}
