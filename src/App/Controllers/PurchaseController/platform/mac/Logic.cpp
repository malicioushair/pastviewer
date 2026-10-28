#include "App/Controllers/PurchaseController/platform/Logic.h"

namespace PlatformDependentLogic {

bool SupportsNativePurchases()
{
	return false;
}

void InitializeBilling(QStringList)
{
}

void LoadTipProducts(QStringList, TipProductsHandler handler)
{
	handler({}, QStringLiteral("Native tip purchases are unavailable on this platform."));
}

void Purchase(QString, PurchaseHandler handler, bool)
{
	handler(PurchaseResult::Failed, QStringLiteral("Native tip purchases are unavailable on this platform."));
}

void CheckOwned(const QString &, std::function<void(bool)> handler)
{
	if (handler)
		handler(false);
}

void QueryPurchasePending(const QString &, std::function<void(PurchasePendingState)> handler)
{
	if (handler)
		handler(PurchasePendingState::Unknown);
}

void SetVerifiedTransactionHandler(std::function<void(QString)>)
{
}

void SetPurchaseEndedHandler(std::function<void(QString, bool, QString)>)
{
}

} // namespace PlatformDependentLogic
