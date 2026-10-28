#include "PurchaseController.h"

#include <optional>

#include <QGuiApplication>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QVariantMap>

#include "glog/logging.h"

#include "App/Controllers/GuiController/TipPromptTracker.h"
#include "App/Controllers/PurchaseController/platform/Logic.h"

namespace {

QStringList NativeTipProductIds()
{
#if defined(TIP_PRODUCT_IDS) && (defined(Q_OS_ANDROID) || defined(Q_OS_APPLE))
	return QString::fromUtf8(TIP_PRODUCT_IDS)
		.split(QLatin1Char(','), Qt::SkipEmptyParts);
#else
	return {};
#endif
}

QString GetNativeProductId(const PurchaseInfo & purchase)
{
	if (!purchase.paid)
		return {};

#if defined(Q_OS_IOS)
	return purchase.appleId;
#elif defined(Q_OS_ANDROID)
	return purchase.googleId;
#else
	return {};
#endif
}

std::optional<QString> GetNativeOwnershipProductId(const PurchaseInfo & purchase)
{
#if defined(Q_OS_IOS)
	return purchase.appleId;
#elif defined(Q_OS_ANDROID)
	return purchase.googleId;
#else
	Q_UNUSED(purchase);
	return std::nullopt;
#endif
}

} // namespace

struct PurchaseController::Impl
{
	PastViewer::TipPromptTracker * tipPromptTracker { nullptr };
	QVariantList tipProducts {};
	bool tipProductsLoading { false };
	bool tipPurchaseInProgress { false };
	bool tourPurchaseInProgress { false };
    int ownershipChecksInFlight { 0 };
    QString purchasingTourId {};
    QString purchasingTourProductId {};
	// Product id -> tour id. Survives a later purchase attempt so Ask to Buy can still unlock the tour.
	QHash<QString, QString> pendingTourByProductId {};
	QSet<QString> pendingMessageProductIds {};
	QSet<QString> pendingRefreshInFlight {};
	QHash<QString, int> pendingGeneration {};
	QSet<QString> reportedTourPurchaseFailureProductIds {};
	QTimer pendingPurchaseTimer {};
	QHash<QString, quint64> tourOwnedRequestGeneration {};
};

PurchaseController::PurchaseController(QObject * parent)
	: QObject(parent)
	, m_impl(std::make_unique<Impl>())
{
	{
		const QPointer<PurchaseController> controller(this);
		PlatformDependentLogic::SetVerifiedTransactionHandler([controller](QString productId) {
			if (false
				|| !controller
				|| productId.isEmpty()
				|| NativeTipProductIds().contains(productId))
				return;

			QString tourId;
			if (productId == controller->m_impl->purchasingTourProductId)
				tourId = controller->m_impl->purchasingTourId;
			if (tourId.isEmpty())
				tourId = controller->m_impl->pendingTourByProductId.value(productId);
			if (tourId.isEmpty())
				return;

			controller->CompletePendingTourPurchase(productId);
			emit controller->tourPurchaseSucceeded(tourId);
		});
	}
	{
		const QPointer<PurchaseController> controller(this);
		PlatformDependentLogic::SetPurchaseEndedHandler([controller](QString productId, bool failed, QString errorMessage) {
			if (!controller || productId.isEmpty() || NativeTipProductIds().contains(productId))
				return;

			auto tourId = controller->m_impl->pendingTourByProductId.value(productId);
			if (tourId.isEmpty() && controller->m_impl->purchasingTourProductId == productId)
				tourId = controller->m_impl->purchasingTourId;
			if (tourId.isEmpty())
				return;

			if (failed)
			{
				controller->FailTourPurchase(productId, tourId, errorMessage);
				return;
			}

			if (!controller->m_impl->pendingMessageProductIds.contains(productId))
				return;

			const auto generation = controller->m_impl->pendingGeneration.value(productId);
			PlatformDependentLogic::CheckOwned(
				productId,
				[controller, productId, tourId, generation](bool owned) {
					if (!controller || !owned)
						return;

					controller->SettlePendingTourPurchase(productId, tourId, generation, true);
				});
		});
	}

	if (qGuiApp)
	{
		connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
			if (state == Qt::ApplicationActive)
				RefreshPendingTourPurchases();
		});
	}

	m_impl->pendingPurchaseTimer.setInterval(2000);
	connect(&m_impl->pendingPurchaseTimer, &QTimer::timeout, this, [this] {
		RefreshPendingTourPurchases();
	});
	PlatformDependentLogic::InitializeBilling(NativeTipProductIds());
}

PurchaseController::~PurchaseController() = default;

void PurchaseController::SetTipPromptTracker(PastViewer::TipPromptTracker * tracker)
{
	m_impl->tipPromptTracker = tracker;
}

bool PurchaseController::HasTipSupport() const
{
	return PlatformDependentLogic::SupportsNativePurchases() && !NativeTipProductIds().isEmpty();
}

void PurchaseController::RequestTipFlow()
{
	if (!PlatformDependentLogic::SupportsNativePurchases())
		return;

	if (NativeTipProductIds().isEmpty())
		return;

	if (m_impl->tipPromptTracker)
		m_impl->tipPromptTracker->DisableAutomaticPrompts();
	emit tipJarRequested();
}

void PurchaseController::LoadTipProducts()
{
	if (false
		|| !PlatformDependentLogic::SupportsNativePurchases()
		|| NativeTipProductIds().isEmpty()
		|| m_impl->tipProductsLoading
		|| !m_impl->tipProducts.isEmpty())
		return;

	m_impl->tipProductsLoading = true;
	emit tipProductsLoadingChanged();

	const QPointer<PurchaseController> guard(this);
	PlatformDependentLogic::LoadTipProducts(
		NativeTipProductIds(),
		[guard](QVector<PlatformDependentLogic::TipProduct> products, const QString & error) {
			if (!guard)
				return;

			guard->m_impl->tipProductsLoading = false;
			emit guard->tipProductsLoadingChanged();

			if (!error.isEmpty() || products.isEmpty())
			{
				LOG(ERROR) << "Failed to load native tip products: "
						   << (error.isEmpty() ? "No configured products were returned" : error.toStdString());
				emit guard->tipOperationFailed();
				return;
			}

			QVariantList productValues;
			productValues.reserve(products.size());
			for (const auto & product : products)
			{
				QVariantMap productValue;
				productValue.insert(QStringLiteral("id"), product.id);
				productValue.insert(QStringLiteral("title"), product.title);
				productValue.insert(QStringLiteral("displayPrice"), product.displayPrice);
				productValues.push_back(productValue);
			}

			guard->m_impl->tipProducts = std::move(productValues);
			emit guard->tipProductsChanged();
		});
}

void PurchaseController::PurchaseTip(const QString & productId)
{
	if (false
		|| !PlatformDependentLogic::SupportsNativePurchases()
		|| m_impl->tipPurchaseInProgress
		|| m_impl->tourPurchaseInProgress
		|| !NativeTipProductIds().contains(productId))
		return;

	m_impl->tipPurchaseInProgress = true;
    emit purchaseInProgressChanged();

    const QPointer<PurchaseController> guard(this);
    PlatformDependentLogic::Purchase(
		productId,
		[guard, productId](PlatformDependentLogic::PurchaseResult result, const QString & error) {
			if (!guard)
				return;

			guard->m_impl->tipPurchaseInProgress = false;
            emit guard->purchaseInProgressChanged();

            switch (result)
            {
				case PlatformDependentLogic::PurchaseResult::Succeeded:
					LOG(INFO) << "Native tip purchase succeeded: " << productId.toStdString();
					if (guard->m_impl->tipPromptTracker)
						guard->m_impl->tipPromptTracker->DisableAutomaticPrompts();
					emit guard->tipPurchaseSucceeded();
					return;
				case PlatformDependentLogic::PurchaseResult::Cancelled:
					return;
				case PlatformDependentLogic::PurchaseResult::Pending:
					emit guard->tipPurchasePending();
					return;
				case PlatformDependentLogic::PurchaseResult::Failed:
					LOG(ERROR) << "Native tip purchase failed: " << error.toStdString();
					emit guard->tipOperationFailed();
					return;
			}
		});
}

QVariantList PurchaseController::TipProducts() const
{
	return m_impl->tipProducts;
}

bool PurchaseController::TipProductsLoading() const
{
	return m_impl->tipProductsLoading;
}

bool PurchaseController::PurchaseInProgress() const
{
    return false
        || m_impl->tipPurchaseInProgress
        || m_impl->tourPurchaseInProgress
        || m_impl->ownershipChecksInFlight > 0;
}

bool PurchaseController::IsTourPurchasePending(const QString & tourId) const
{
	if (tourId.isEmpty())
		return false;

	for (const auto & productId : m_impl->pendingMessageProductIds)
		if (m_impl->pendingTourByProductId.value(productId) == tourId)
			return true;
	return false;
}

void PurchaseController::NotePendingTourPurchase(const QString & productId, const QString & tourId)
{
	m_impl->pendingTourByProductId.insert(productId, tourId);
	m_impl->pendingMessageProductIds.insert(productId);
	m_impl->pendingGeneration.insert(productId, m_impl->pendingGeneration.value(productId) + 1);
	if (!m_impl->pendingPurchaseTimer.isActive())
		m_impl->pendingPurchaseTimer.start();
}

void PurchaseController::ClearPendingTourMessage(const QString & productId)
{
	m_impl->pendingMessageProductIds.remove(productId);
	m_impl->pendingRefreshInFlight.remove(productId);
	if (m_impl->pendingMessageProductIds.isEmpty())
		m_impl->pendingPurchaseTimer.stop();
}

void PurchaseController::CompletePendingTourPurchase(const QString & productId)
{
	if (m_impl->purchasingTourProductId == productId)
	{
		m_impl->purchasingTourId.clear();
		m_impl->purchasingTourProductId.clear();
	}
	m_impl->pendingTourByProductId.remove(productId);
	m_impl->reportedTourPurchaseFailureProductIds.remove(productId);
	ClearPendingTourMessage(productId);
}

void PurchaseController::FailTourPurchase(const QString & productId, const QString & tourId, const QString & errorMessage)
{
	if (productId.isEmpty() || tourId.isEmpty())
		return;
	if (m_impl->reportedTourPurchaseFailureProductIds.contains(productId))
		return;

	m_impl->reportedTourPurchaseFailureProductIds.insert(productId);
	LOG(ERROR) << "Tour purchase failed: " << errorMessage.toStdString();

	// Keep the product map so a later verified transaction can still unlock this tour.
	// An actual store failure clears the pending message and shows the error.
	if (m_impl->pendingMessageProductIds.contains(productId))
	{
		m_impl->pendingGeneration.insert(productId, m_impl->pendingGeneration.value(productId) + 1);
		ClearPendingTourMessage(productId);
		emit tourPurchasePendingCleared(tourId);
	}
	if (m_impl->purchasingTourProductId == productId)
	{
		m_impl->purchasingTourId.clear();
		m_impl->purchasingTourProductId.clear();
	}

	emit showErrorDialog(
		errorMessage.isEmpty() ? tr("The App Store could not complete this purchase.") : errorMessage);
}

void PurchaseController::SettlePendingTourPurchase(const QString & productId, const QString & tourId, int generation, bool owned)
{
	if (!owned)
		return;
	if (m_impl->pendingGeneration.value(productId) != generation)
		return;
	if (!m_impl->pendingMessageProductIds.contains(productId))
		return;
	if (m_impl->pendingTourByProductId.value(productId) != tourId)
		return;

	CompletePendingTourPurchase(productId);
	emit tourPurchaseSucceeded(tourId);
}

void PurchaseController::RefreshPendingTourPurchases()
{
	const auto productIds = m_impl->pendingMessageProductIds.values();
	for (const auto & productId : productIds)
	{
		if (m_impl->pendingRefreshInFlight.contains(productId))
			continue;

		const auto tourId = m_impl->pendingTourByProductId.value(productId);
		if (tourId.isEmpty())
		{
			ClearPendingTourMessage(productId);
			continue;
		}

		const auto generation = m_impl->pendingGeneration.value(productId);
		m_impl->pendingRefreshInFlight.insert(productId);
		const QPointer<PurchaseController> guard(this);
		// Finish any verified transaction, then keep the pending message until
		// a verified entitlement exists. An empty Transaction.unfinished queue
		// is not enough to clear it, including after the tour page is opened again.
		PlatformDependentLogic::QueryPurchasePending(
			productId,
			[guard, productId, tourId, generation](PlatformDependentLogic::PurchasePendingState) {
				if (!guard)
					return;

				guard->m_impl->pendingRefreshInFlight.remove(productId);
				if (guard->m_impl->pendingGeneration.value(productId) != generation)
				{
					guard->RefreshPendingTourPurchases();
					return;
				}
				if (!guard->m_impl->pendingMessageProductIds.contains(productId))
					return;
				if (guard->m_impl->pendingTourByProductId.value(productId) != tourId)
					return;

				guard->m_impl->pendingRefreshInFlight.insert(productId);
				PlatformDependentLogic::CheckOwned(
					productId,
					[guard, productId, tourId, generation](bool owned) {
						if (!guard)
							return;

						guard->m_impl->pendingRefreshInFlight.remove(productId);
						if (guard->m_impl->pendingGeneration.value(productId) != generation)
						{
							guard->RefreshPendingTourPurchases();
							return;
						}
						if (!owned)
							return;

						guard->SettlePendingTourPurchase(productId, tourId, generation, true);
					});
			});
	}
}

void PurchaseController::RequestTourPurchaseFlow(const DisplayTour & tour)
{
	if (!PlatformDependentLogic::SupportsNativePurchases())
		return;

	const auto productId = GetNativeProductId(tour.purchaseInfo);
	if (productId.isEmpty())
		return;

	StartTourPurchase(tour.id, productId);
}

void PurchaseController::StartTourPurchase(const QString & tourId, const QString & productId)
{
	// Pending Ask to Buy stays blocked until a verified entitlement exists.
	// A tip already in progress must not swallow this tap: the store reports that failure.
	if (false
		|| !PlatformDependentLogic::SupportsNativePurchases()
		|| m_impl->tourPurchaseInProgress
		|| m_impl->pendingMessageProductIds.contains(productId)
		|| productId.isEmpty())
		return;

	m_impl->reportedTourPurchaseFailureProductIds.remove(productId);
	m_impl->purchasingTourId = tourId;
	m_impl->purchasingTourProductId = productId;
	m_impl->tourPurchaseInProgress = true;
    emit purchaseInProgressChanged();

    const QPointer<PurchaseController> guard(this);
    PlatformDependentLogic::Purchase(
		productId,
		[guard, productId, tourId](PlatformDependentLogic::PurchaseResult result, const QString & error) {
			if (!guard)
				return;

			if (result == PlatformDependentLogic::PurchaseResult::Pending)
				guard->NotePendingTourPurchase(productId, tourId);

			guard->m_impl->tourPurchaseInProgress = false;
            emit guard->purchaseInProgressChanged();

            const auto clearPurchase = [guard, productId] {
                if (guard->m_impl->purchasingTourProductId != productId)
					return;
				if (guard->m_impl->pendingMessageProductIds.contains(productId))
					return;

				guard->m_impl->purchasingTourId.clear();
				guard->m_impl->purchasingTourProductId.clear();
            };
            switch (result)
			{
				case PlatformDependentLogic::PurchaseResult::Succeeded:
					LOG(INFO) << "Tour purchase succeeded: " << productId.toStdString();
					guard->CompletePendingTourPurchase(productId);
					emit guard->tourPurchaseSucceeded(tourId);
					return;
				case PlatformDependentLogic::PurchaseResult::Cancelled:
					clearPurchase();
					return;
				case PlatformDependentLogic::PurchaseResult::Pending:
					emit guard->tourPurchasePending(tourId);
					guard->RefreshPendingTourPurchases();
					return;
				case PlatformDependentLogic::PurchaseResult::Failed:
					guard->FailTourPurchase(productId, tourId, error);
					return;
			}
		},
		true);
}

void PurchaseController::CheckTourOwned(const DisplayTour & tour)
{
	// A later check for the same tour supersedes a slower in-flight result.
	const auto requestGeneration = ++m_impl->tourOwnedRequestGeneration[tour.id];
    ++m_impl->ownershipChecksInFlight;
    emit purchaseInProgressChanged();

    const QPointer<PurchaseController> guard(this);
    const auto publish = [guard, tourId = tour.id, requestGeneration](bool owned) {
		if (!guard)
			return;

        if (guard->m_impl->ownershipChecksInFlight > 0)
            --guard->m_impl->ownershipChecksInFlight;
        emit guard->purchaseInProgressChanged();

        if (guard->m_impl->tourOwnedRequestGeneration.value(tourId) != requestGeneration)
            return;
		emit guard->tourOwned(tourId, owned);
	};

	if (!tour.purchaseInfo.paid)
	{
		publish(true);
		return;
	}

	const auto productId = GetNativeOwnershipProductId(tour.purchaseInfo);
	if (!productId)
	{
		publish(false);
		return;
	}

#if defined(Q_OS_IOS)
	// An empty apple id is owned and does not query the store.
	if (productId->isEmpty())
	{
		publish(true);
		return;
	}
#endif
	PlatformDependentLogic::CheckOwned(*productId, publish);
}
