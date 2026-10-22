#include "App/Controllers/PurchaseController/platform/Logic.h"

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <utility>

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>

#include "App/Controllers/PurchaseController/platform/ios/StoreKitBridge.h"

namespace {
std::mutex callbackMutex;
PlatformDependentLogic::TipProductsHandler tipProductsHandler;
PlatformDependentLogic::PurchaseHandler purchaseHandler;
std::unordered_map<uint64_t, std::function<void(bool)>> ownershipHandlers;
std::unordered_map<uint64_t, std::function<void(PlatformDependentLogic::PurchasePendingState)>> pendingHandlers;
uint64_t nextOwnershipRequestId = 1;
std::function<void(QString)> verifiedTransactionHandler;
std::function<void(QString, bool, QString)> purchaseEndedHandler;

template<typename Callback, typename... Args>
void DispatchToQt(Callback callback, Args... args)
{
	if (!callback)
		return;

	auto invoke = [callback = std::move(callback), ... args = std::move(args)]() mutable {
		callback(std::move(args)...);
	};
	if (auto * application = QCoreApplication::instance())
		QMetaObject::invokeMethod(application, std::move(invoke), Qt::QueuedConnection);
	else
		invoke();
}

void StoreKitProductsLoaded(const char * productsJson, const char * errorMessage)
{
	PlatformDependentLogic::TipProductsHandler handler;
	{
		const std::scoped_lock lock(callbackMutex);
		handler = std::move(tipProductsHandler);
	}

	QVector<PlatformDependentLogic::TipProduct> products;
	QString error = errorMessage ? QString::fromUtf8(errorMessage) : QString {};
	if (productsJson)
	{
		QJsonParseError parseError;
		const auto document = QJsonDocument::fromJson(QByteArray(productsJson), &parseError);
		if (parseError.error != QJsonParseError::NoError || !document.isArray())
			error = QStringLiteral("Could not parse StoreKit product data: %1").arg(parseError.errorString());
		else
		{
			for (const auto & value : document.array())
			{
				const auto object = value.toObject();
				const auto id = object.value(QStringLiteral("id")).toString();
				const auto title = object.value(QStringLiteral("title")).toString();
				const auto displayPrice = object.value(QStringLiteral("displayPrice")).toString();
				if (!id.isEmpty() && !displayPrice.isEmpty())
					products.push_back({ id, title, displayPrice });
			}
		}
	}

	DispatchToQt(std::move(handler), std::move(products), std::move(error));
}

void StoreKitPurchaseFinished(int result, const char * errorMessage)
{
	PlatformDependentLogic::PurchaseHandler handler;
	{
		const std::scoped_lock lock(callbackMutex);
		handler = std::move(purchaseHandler);
	}

	auto purchaseResult = PlatformDependentLogic::PurchaseResult::Failed;
	switch (result)
	{
	case 0:
		purchaseResult = PlatformDependentLogic::PurchaseResult::Succeeded;
		break;
	case 1:
		purchaseResult = PlatformDependentLogic::PurchaseResult::Cancelled;
		break;
	case 2:
		purchaseResult = PlatformDependentLogic::PurchaseResult::Pending;
		break;
	default:
		break;
	}

	DispatchToQt(
		std::move(handler),
		purchaseResult,
		errorMessage ? QString::fromUtf8(errorMessage) : QString {});
}

void StoreKitOwnershipChecked(uint64_t requestId, int owned)
{
	std::function<void(bool)> handler;
	{
		const std::scoped_lock lock(callbackMutex);
		const auto it = ownershipHandlers.find(requestId);
		if (it == ownershipHandlers.end())
			return;

		handler = std::move(it->second);
		ownershipHandlers.erase(it);
	}

	DispatchToQt(std::move(handler), owned != 0);
}

void StoreKitPendingChecked(uint64_t requestId, int state)
{
	std::function<void(PlatformDependentLogic::PurchasePendingState)> handler;
	{
		const std::scoped_lock lock(callbackMutex);
		const auto it = pendingHandlers.find(requestId);
		if (it == pendingHandlers.end())
			return;

		handler = std::move(it->second);
		pendingHandlers.erase(it);
	}

	using PlatformDependentLogic::PurchasePendingState;
	const auto pendingState = [state] {
		if (state > 0)
			return PurchasePendingState::Pending;
		if (state == 0)
			return PurchasePendingState::NotPending;
		return PurchasePendingState::Unknown;
	}();
	DispatchToQt(std::move(handler), pendingState);
}

void StoreKitTransactionUpdated(const char * productId)
{
	std::function<void(QString)> handler;
	{
		const std::scoped_lock lock(callbackMutex);
		handler = verifiedTransactionHandler;
	}

	DispatchToQt(std::move(handler), productId ? QString::fromUtf8(productId) : QString {});
}

void StoreKitPurchaseEnded(const char * productId, int failed, const char * errorMessage)
{
	std::function<void(QString, bool, QString)> handler;
	{
		const std::scoped_lock lock(callbackMutex);
		handler = purchaseEndedHandler;
	}

	DispatchToQt(
		std::move(handler),
		productId ? QString::fromUtf8(productId) : QString {},
		failed != 0,
		errorMessage ? QString::fromUtf8(errorMessage) : QString {});
}
}

namespace PlatformDependentLogic {

bool SupportsNativePurchases()
{
	return true;
}

void InitializeBilling(QStringList)
{
}

void LoadTipProducts(QStringList productIds, TipProductsHandler handler)
{
	bool requestInProgress = false;
	{
		const std::scoped_lock lock(callbackMutex);
		requestInProgress = static_cast<bool>(tipProductsHandler);
		// Replace any in-flight waiter so the outstanding StoreKit query delivers to the latest caller.
		tipProductsHandler = std::move(handler);
	}
	if (requestInProgress)
		return;

	const auto productIdsCsv = productIds.join(QLatin1Char(',')).toUtf8();
	PastViewerStoreKitLoadProducts(productIdsCsv.constData(), StoreKitProductsLoaded);
}

void Purchase(QString productId, PurchaseHandler handler, bool nonConsumable)
{
	bool purchaseInProgress = false;
	{
		const std::scoped_lock lock(callbackMutex);
		if (purchaseHandler)
			purchaseInProgress = true;
		else
			purchaseHandler = std::move(handler);
	}
	if (purchaseInProgress)
	{
		handler(PurchaseResult::Failed, QStringLiteral("A StoreKit purchase is already in progress."));
		return;
	}

	const auto productIdUtf8 = productId.toUtf8();
	PastViewerStoreKitPurchase(productIdUtf8.constData(), nonConsumable ? 1 : 0, StoreKitPurchaseFinished);
}

void CheckOwned(const QString & productId, std::function<void(bool)> handler)
{
	if (!handler)
		return;

	if (productId.isEmpty())
	{
		handler(false);
		return;
	}

	uint64_t requestId = 0;
	{
		const std::scoped_lock lock(callbackMutex);
		requestId = nextOwnershipRequestId++;
		ownershipHandlers.emplace(requestId, std::move(handler));
	}

	const auto productIdUtf8 = productId.toUtf8();
	PastViewerStoreKitCheckOwned(productIdUtf8.constData(), requestId, StoreKitOwnershipChecked);
}

void QueryPurchasePending(const QString & productId, std::function<void(PurchasePendingState)> handler)
{
	if (!handler)
		return;

	if (productId.isEmpty())
	{
		handler(PurchasePendingState::NotPending);
		return;
	}

	uint64_t requestId = 0;
	{
		const std::scoped_lock lock(callbackMutex);
		requestId = nextOwnershipRequestId++;
		pendingHandlers.emplace(requestId, std::move(handler));
	}

	const auto productIdUtf8 = productId.toUtf8();
	PastViewerStoreKitQueryPending(productIdUtf8.constData(), requestId, StoreKitPendingChecked);
}

void SetVerifiedTransactionHandler(std::function<void(QString)> handler)
{
	{
		const std::scoped_lock lock(callbackMutex);
		verifiedTransactionHandler = std::move(handler);
	}

	PastViewerStoreKitStartTransactionListener(StoreKitTransactionUpdated);
}

void SetPurchaseEndedHandler(std::function<void(QString, bool, QString)> handler)
{
	{
		const std::scoped_lock lock(callbackMutex);
		purchaseEndedHandler = std::move(handler);
	}

	PastViewerStoreKitStartPurchaseEndedListener(StoreKitPurchaseEnded);
}

} // namespace PlatformDependentLogic
