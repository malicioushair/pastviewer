#pragma once

#include <functional>

#include <QString>
#include <QStringList>
#include <QVector>

namespace PlatformDependentLogic {

struct TipProduct
{
    QString id;
    QString title;
    QString displayPrice;
};

enum class PurchaseResult
{
    Succeeded,
    Cancelled,
    Pending,
    Failed,
};

enum class PurchasePendingState
{
    Unknown,
    Pending,
    NotPending,
};

using TipProductsHandler = std::function<void(QVector<TipProduct>, QString)>;
using PurchaseHandler = std::function<void(PurchaseResult, QString)>;

bool SupportsNativePurchases();
void InitializeBilling(QStringList productIds);
void LoadTipProducts(QStringList productIds, TipProductsHandler handler);
void Purchase(QString productId, PurchaseHandler handler, bool nonConsumable = false);
void CheckOwned(const QString & productId, std::function<void(bool)> handler);
void QueryPurchasePending(const QString & productId, std::function<void(PurchasePendingState)> handler);
void SetVerifiedTransactionHandler(std::function<void(QString productId)> handler);
void SetPurchaseEndedHandler(std::function<void(QString productId, bool failed, QString errorMessage)> handler);

} // namespace PlatformDependentLogic
