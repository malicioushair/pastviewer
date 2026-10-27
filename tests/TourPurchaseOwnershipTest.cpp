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

    // Leak the application for process lifetime. Controllers can leave
    // QObjects that crash if the application is destroyed first.
    static int argc = 1;
    static char executableName[] = "PastViewerTests";
    static char * argv[] = { executableName, nullptr };
    static auto * const application = new QCoreApplication(argc, argv);
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
concept HasPurchaseInProgressApi = requires(Controller & controller) {
    { controller.PurchaseInProgress() } -> std::same_as<bool>;
    &Controller::purchaseInProgressChanged;
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

// Observes purchaseInProgress across a CheckTourOwned call. Desktop ownership
// does not use StoreKit; the flag must still go true for the in-flight query
// and be false again by the time tourOwned is delivered.
struct PurchaseInProgressDuringOwnership
{
    std::optional<OwnershipEmission> emission;
    bool sawPurchaseInProgress = false;
    bool purchaseInProgressWhenOwned = true;
};

template <typename Controller>
std::optional<PurchaseInProgressDuringOwnership>
WaitForTourOwnedObservingPurchaseInProgress(Controller & controller, const DisplayTour & tour)
{
    if constexpr (!HasTourOwnershipApi<Controller> || !HasPurchaseInProgressApi<Controller>)
    {
        ADD_FAILURE()
            << "PurchaseController must expose CheckTourOwned, tourOwned, "
               "PurchaseInProgress(), and purchaseInProgressChanged";
        return std::nullopt;
    }
    else
    {
        PurchaseInProgressDuringOwnership observed;
        QEventLoop loop;
        const auto progressConnection = QObject::connect(
            &controller,
            &Controller::purchaseInProgressChanged,
            &loop,
            [&]() {
                if (controller.PurchaseInProgress())
                    observed.sawPurchaseInProgress = true;
            });
        const auto ownedConnection = QObject::connect(
            &controller,
            &Controller::tourOwned,
            &loop,
            [&](const QString & tourId, bool owned) {
                observed.emission = OwnershipEmission { tourId, owned };
                observed.purchaseInProgressWhenOwned = controller.PurchaseInProgress();
                loop.quit();
            });

        controller.CheckTourOwned(tour);
        if (!observed.emission)
        {
            QTimer timeout;
            timeout.setSingleShot(true);
            QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeout.start(kOwnershipSignalTimeoutMs);
            loop.exec();
        }

        QObject::disconnect(progressConnection);
        QObject::disconnect(ownedConnection);
        return observed;
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

// Template so discarded if-constexpr branches stay dependent on Controller and
// compile before purchaseInProgress replaces tipPurchaseInProgress.
template <typename Controller>
void ExpectPurchaseInProgressTracksCheckTourOwned(Controller & controller)
{
    if constexpr (!HasPurchaseInProgressApi<Controller>)
    {
        GTEST_FAIL()
            << "PurchaseController must expose purchaseInProgress "
               "(PurchaseInProgress / purchaseInProgressChanged), not tipPurchaseInProgress";
    }
    else
    {
        EXPECT_FALSE(controller.PurchaseInProgress())
            << "purchaseInProgress must be false at rest";

        const auto tour = MakeTour(QStringLiteral("paid-progress"), true);
        const auto observed = WaitForTourOwnedObservingPurchaseInProgress(controller, tour);

        ASSERT_TRUE(observed.has_value());
        ASSERT_TRUE(observed->emission.has_value())
            << "CheckTourOwned did not emit tourOwned within " << kOwnershipSignalTimeoutMs << "ms";
        EXPECT_EQ(observed->emission->tourId, tour.id);
        EXPECT_FALSE(observed->emission->owned)
            << "a paid tour on desktop must emit tourOwned with owned false";
        EXPECT_TRUE(observed->sawPurchaseInProgress)
            << "purchaseInProgress must become true while CheckTourOwned is in flight";
        EXPECT_FALSE(observed->purchaseInProgressWhenOwned)
            << "purchaseInProgress must be false by the time tourOwned is observed";
        EXPECT_FALSE(controller.PurchaseInProgress())
            << "purchaseInProgress must remain false after ownership settles";
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

TEST_F(TourPurchaseOwnership, PurchaseInProgressTracksCheckTourOwnedOnDesktop)
{
#if !PASTVIEWER_HAS_PURCHASE_CONTROLLER
    GTEST_FAIL() << "PurchaseController is not implemented; ownership must move off TourController";
#else
    ASSERT_NE(controller, nullptr);
    ExpectPurchaseInProgressTracksCheckTourOwned(*controller);
#endif
}
